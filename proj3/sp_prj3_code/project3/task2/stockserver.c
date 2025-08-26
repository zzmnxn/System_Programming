/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define MAX_CLIENT 100
#define NTHREADS 50
#define SBUFSIZE 50

typedef struct stock {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    struct stock *left, *right;
    sem_t w;
} stock_t;

stock_t *root = NULL;


int connected_clients = 0;
sem_t client_count_mutex;

typedef struct {
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;

} sbuf_t;

sbuf_t sbuf;

void echo(int connfd);
void insert_stock(stock_t **root, int id, int left, int price);
void load_stock_data(const char *filename);
void collect_show(stock_t *node, char *buf);
void show_stocks(int connfd);
void buy_stock(int connfd, int id, int amount);
void sell_stock(int connfd, int id, int amount);
void handle_command(int connf);
void save_stock_data(const char *filename);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_insert(sbuf_t *sp, int item);
void *thread(void *vargp);
int sbuf_remove(sbuf_t *sp);

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


int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    pthread_t tid;
    
    //char client_hostname[MAXLINE], client_port[MAXLINE];
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port> \n", argv[0]);
	exit(0);
    }

    //데이터 로드
    load_stock_data("stock.txt");
    //서버포트 열기
    listenfd = Open_listenfd(argv[1]);
    // 초기화
    sbuf_init(&sbuf, SBUFSIZE);
    Sem_init(&client_count_mutex, 0, 1);
    // 워커 스레드 생성
    for (int i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);


    //클라이언트 반복 처리리
   while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);
        char client_hostname[MAXLINE], client_port[MAXLINE];
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
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
        P(&cur->mutex);
        cur->readcnt++;
        if (cur->readcnt == 1) P(&cur->w);  // 첫 번째 리더가 write 막음
        V(&cur->mutex);
        sprintf(line, "%d %d %d\n", cur->ID, cur->left_stock, cur->price);
        strcat(buf, line);
        P(&cur->mutex);
        cur->readcnt--;
        if (cur->readcnt == 0) V(&cur->w);  // 마지막 리더가 write 허용
        V(&cur->mutex);

        if (cur->left) queue[rear++] = cur->left;
        if (cur->right) queue[rear++] = cur->right;
    }
}

void show_stocks(int connfd) {
    char buf[MAXLINE] = "";
    collect_show(root, buf);
    Rio_writen(connfd, buf, strlen(buf));
}

void buy_stock(int connfd, int id, int amount) {
    stock_t *curr = root;
    while (curr) {
        if (id == curr->ID) {
            P(&curr->w);
            if (curr->left_stock < amount) {
                Rio_writen(connfd, "Not enough left stocks\n", strlen("Not enough left stocks\n"));
            } else {
                curr->left_stock -= amount;
                Rio_writen(connfd, "[buy] success\n", strlen("[buy] success\n"));
            }
            V(&curr->w);
            return;
        }
        curr = (id < curr->ID) ? curr->left : curr->right;
    }
   
}

void sell_stock(int connfd, int id, int amount) {
    stock_t *curr = root;
    while (curr) {
        if (id == curr->ID) {
            P(&curr->w);
            curr->left_stock += amount;
            Rio_writen(connfd, "[sell] success\n", strlen("[sell] success\n"));
           V(&curr->w);
            return;
        }
        curr = (id < curr->ID) ? curr->left : curr->right;
    }
}

void save_stock(FILE *fp, stock_t *node) {
    if (!node) return;
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

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        // 클라이언트 수 증가
        P(&client_count_mutex);
        connected_clients++;
        V(&client_count_mutex);
        handle_command(connfd);
        Close(connfd);
        // 클라이언트 수 감소
        P(&client_count_mutex);
        connected_clients--;
        if (connected_clients == 0) {
            save_stock_data("stock.txt");
            
        }
        V(&client_count_mutex);
    }
}

void handle_command(int connfd) {
    rio_t rio;
    char buf[MAXLINE];
    char cmd[16];
    int id, amount;

    Rio_readinitb(&rio, connfd);
    while (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        char reply[MAXLINE] = "";

        if (sscanf(buf, "%s", cmd) == 1) {
            if (!strcmp(cmd, "show")) {
                collect_show(root, reply);
                
            
            } 
            else if (strcmp(cmd, "buy") == 0){
                sscanf(buf, "%*s %d %d", &id, &amount);
                stock_t *curr = find_stock_by_id(id);
                if (curr) {
                    if (curr->ID == id) {
                        P(&curr->w);
                        if (curr->left_stock < amount) {
                            strcpy(reply, "Not enough left stocks\n");
                        } else {
                            curr->left_stock -= amount;
                            strcpy(reply, "[buy] success\n");
                                
                            
                        }
                        V(&curr->w);
                    }
                }
            
            } else if (!strcmp(cmd, "sell")){
                sscanf(buf, "%*s %d %d", &id, &amount);
                stock_t *curr = find_stock_by_id(id);
                if(curr) {
                    if (curr->ID == id) {
                        P(&curr->w);
                        curr->left_stock += amount;
                        strcpy(reply, "[sell] success\n");
                        V(&curr->w);
                        
                    }
                    
                }
    
            } else if (strcmp(cmd, "exit") == 0) return;
            char padded[MAXLINE] = {0};
            strncpy(padded, reply, MAXLINE - 1);
            Rio_writen(connfd, padded, MAXLINE);

        }
    }
}


void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);
    P(&sp->mutex);
    sp->rear = (sp->rear + 1) % sp->n;
    sp->buf[sp->rear] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);
    P(&sp->mutex);
    sp->front= (sp->front +1)% sp->n;
    item= sp->buf[sp->front];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}