/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define MAX_CLIENT 100
int connected_clients = 0;
sem_t client_count_mutex;

typedef struct stock {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    sem_t w;
    struct stock *left, *right;
} stock_t;

stock_t *root = NULL;

void insert_stock(stock_t **root, int id, int left, int price);
void load_stock_data(const char *filename);
void handle_command(int connfd, char * buf);
void save_stock_data(const char *filename);

stock_t *find_stock_by_id(int id) {
    stock_t *queue[100];
    int front = 0, rear = 0;
    if (!root) return NULL;
    queue[rear++] = root;

    while (front < rear) {
        stock_t *curr = queue[front++];
        if (curr->ID == id) return curr;
        if (curr->left) queue[rear++] = curr->left;
        if (curr->right) queue[rear++] = curr->right;
    }
    return NULL;
}

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int clientfd[MAX_CLIENT];
    rio_t clientrio[MAX_CLIENT];
} pool;

void init_pool(int listenfd, pool *p) {
    p->maxfd = listenfd;
    for (int i = 0; i < MAX_CLIENT; i++)
        p->clientfd[i] = -1;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (p->clientfd[i] < 0) {
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);
            FD_SET(connfd, &p->read_set);
            if (connfd > p->maxfd) p->maxfd = connfd;
            P(&client_count_mutex);
            connected_clients++;
            V(&client_count_mutex);
            break;
        }
    }
}

void check_clients(pool *p) {
    char buf[MAXLINE];
    for (int i = 0; i < MAX_CLIENT && p->nready > 0; i++) {
        int connfd = p->clientfd[i];
        if (connfd < 0) continue;

        if (FD_ISSET(connfd, &p->ready_set)) {
            int n = Rio_readlineb(&p->clientrio[i], buf, MAXLINE);
            if (n > 0) {
                handle_command(connfd, buf);
            } else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
                P(&client_count_mutex);
                connected_clients--;
                if (connected_clients == 0) {
                    save_stock_data("stock.txt");
                    
                }
                V(&client_count_mutex);
            }
            p->nready--;
        }
    }
}

int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage

    char client_hostname[MAXLINE], client_port[MAXLINE];
    pool pool;
    
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port> \n", argv[0]);
	exit(0);
    }

    //데이터 로드
    load_stock_data("stock.txt");
    //서버포트 열기
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd,&pool);

    Sem_init(&client_count_mutex, 0, 1);

    //클라이언트 반복 처리리
   while (1) {
        pool.ready_set=pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) {
            clientlen = sizeof(clientaddr);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

            Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool);
            pool.nready--;
        }

        check_clients(&pool);
        
    }

    return 0;
}

void insert_stock(stock_t **root, int id, int left_stock, int price) {
    stock_t *new_node = malloc(sizeof(stock_t));
    new_node->ID = id;
    new_node->left_stock = left_stock;
    new_node->price = price;
    sem_init(&new_node->mutex, 0, 1);
    sem_init(&new_node->w,0,1);
    new_node->readcnt=0;
    new_node->left = new_node->right = NULL;

    if (*root == NULL) {
        *root = new_node;
        return;
    }
    stock_t *queue[100];
    int front = 0, rear = 0;
    queue[rear++] = *root;

    while (front < rear) {
        stock_t *cur = queue[front++];

        if (cur->left == NULL) {
            cur->left = new_node;
            return;
        } else queue[rear++] = cur->left;

        if (cur->right == NULL) {
            cur->right = new_node;
            return;
        } else queue[rear++] = cur->right;
    }
}


void load_stock_data(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    int id, left_stock, price;
    while (fscanf(fp, "%d %d %d", &id, &left_stock, &price) != EOF) {
        insert_stock(&root, id, left_stock, price);
    }

    fclose(fp);
}

void collect_show(stock_t *root, char *buf) {
    if (root == NULL) return;

    stock_t *queue[100];
    int front = 0, rear = 0;
    queue[rear++] = root;

    char line[128];
    while (front < rear) {
        stock_t *cur = queue[front++];
        sprintf(line, "%d %d %d\n", cur->ID, cur->left_stock, cur->price);
        strcat(buf, line);

        if (cur->left) queue[rear++] = cur->left;
        if (cur->right) queue[rear++] = cur->right;
    }
}


void show_stocks(int connfd) {
    char buf[MAXLINE] = "";
    collect_show(root, buf);
    Rio_writen(connfd, buf, strlen(buf));
}


void handle_command(int connfd, char *buf) {
    char cmd[16];
    int id, amount;
    char reply[MAXLINE] = "";

    if (sscanf(buf, "%s", cmd) == 1) {
        if (strcmp(cmd, "show") == 0) {
            collect_show(root, reply);

        } else if (strcmp(cmd, "buy") == 0 && sscanf(buf, "%*s %d %d", &id, &amount) == 2) {
            stock_t *curr = find_stock_by_id(id);
            if(curr) {
                P(&curr->w);
                if (curr->left_stock < amount) {
                    strcpy(reply, "Not enough left stocks\n");
                } else {
                    curr->left_stock -= amount;
                    strcpy(reply, "[buy] success\n");
                }
                V(&curr->w);
            }
        } else if (strcmp(cmd, "sell") == 0 && sscanf(buf, "%*s %d %d", &id, &amount) == 2) {
            stock_t *curr = find_stock_by_id(id);
            if(curr) {
                P(&curr->w);
                curr->left_stock += amount;
                strcpy(reply, "[sell] success\n");
                V(&curr->w);
            }
        } else if (strcmp(cmd, "exit") == 0) {
            return;
        }
///패딩 추가해야 출력됨됨
        char padded[MAXLINE] = {0};
        strncpy(padded, reply, MAXLINE - 1);
        Rio_writen(connfd, padded, MAXLINE);
    }
}


void save_stock(FILE *fp, stock_t *node) {
    if (node == NULL) return;
    save_stock(fp, node->left);
    fprintf(fp, "%d %d %d\n", node->ID, node->left_stock, node->price);
    save_stock(fp, node->right);
}

void save_stock_data(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) return;
    stock_t *queue[100];
    int front = 0, rear = 0;
    queue[rear++] = root;

    while (front < rear) {
        stock_t *cur = queue[front++];
        fprintf(fp, "%d %d %d\n", cur->ID, cur->left_stock, cur->price);

        if (cur->left) queue[rear++] = cur->left;
        if (cur->right) queue[rear++] = cur->right;
    }

    fclose(fp);
}

