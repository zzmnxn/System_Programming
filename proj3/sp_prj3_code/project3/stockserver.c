/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#define MAX_CLIENT 100

typedef struct stock {
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    struct stock *left, *right;
} stock_t;

stock_t *root = NULL;

int client_fd[MAX_CLIENT];       // 클라이언트 소켓 목록
fd_set all_fds, ready_fds;       // select용 fd set
int maxfd;                       // select용 max fd


void echo(int connfd);
void insert_stock(stock_t **root, int id, int left, int price);
void load_stock_data(const char *filename);
void collect_show(stock_t *node, char *buf);
void show_stocks(int connfd);
void buy_stock(int connfd, int id, int amount);
void sell_stock(int connfd, int id, int amount);
void handle_command(int connfd, char * buf);
void save_stock_data(const char *filename);
void handle_sigint(int sig);


int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char buf[MAXLINE];
    rio_t client_rio[MAX_CLIENT];

    fd_set all_fds, ready_fds;
    int maxfd, max_i, i, n;
    
    char client_hostname[MAXLINE], client_port[MAXLINE];
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port> \n", argv[0]);
	exit(0);
    }
    signal(SIGINT, handle_sigint);

    //데이터 로드
    load_stock_data("stock.txt");
    //서버포트 열기
    listenfd = Open_listenfd(argv[1]);
    // 초기화
    maxfd = listenfd;
    max_i = -1;
    for (i = 0; i < MAX_CLIENT; i++) client_fd[i] = -1;

    FD_ZERO(&all_fds);
    FD_SET(listenfd, &all_fds);

    //클라이언트 반복 처리리
   while (1) {
        ready_fds = all_fds;

        n = select(maxfd + 1, &ready_fds, NULL, NULL, NULL);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("select error");
            exit(1);
        }


        // 새로운 연결 요청 처리
        if (FD_ISSET(listenfd, &ready_fds)) {
            clientlen = sizeof(clientaddr);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

            Getnameinfo((SA *)&clientaddr, clientlen,
                        client_hostname, MAXLINE,
                        client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);

            // client_fd에 등록
            for (i = 0; i < MAX_CLIENT; i++) {
                if (client_fd[i] < 0) {
                    client_fd[i] = connfd;
                    Rio_readinitb(&client_rio[i], connfd);
                    break;
                }
            }

            if (i == MAX_CLIENT) {
                fprintf(stderr, "Too many clients\n");
                Close(connfd);
                continue;
            }

            FD_SET(connfd, &all_fds);
            if (connfd > maxfd) maxfd = connfd;
            if (i > max_i) max_i = i;

            if (--n <= 0) continue;
        }

        // 클라이언트 요청 처리
        for (i = 0; i <= max_i; i++) {
            int sockfd = client_fd[i];
            if (sockfd < 0) continue;

            if (FD_ISSET(sockfd, &ready_fds)) {
                if ((n = Rio_readlineb(&client_rio[i], buf, MAXLINE)) > 0) {
                    handle_command(sockfd, buf);
                } else {
                    Close(sockfd);
                    FD_CLR(sockfd, &all_fds);
                    client_fd[i] = -1;
                }

                if (--n <= 0) break;
            }
        }
    }

    return 0;
}

void insert_stock(stock_t **root, int id, int left_stock, int price) {
    if (*root == NULL) {
        *root = (stock_t *)malloc(sizeof(stock_t));
        (*root)->ID = id;
        (*root)->left_stock = left_stock;
        (*root)->price = price;
        (*root)->readcnt = 0;
        sem_init(&(*root)->mutex, 0, 1);
        (*root)->left = (*root)->right = NULL;
        return;
    }

    if (id < (*root)->ID) {
        insert_stock(&(*root)->left, id, left_stock, price);
    } else {
        insert_stock(&(*root)->right, id, left_stock, price);
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

void collect_show(stock_t *node, char *buf) {
    char line[128];
    if (node == NULL) return;

    collect_show(node->left, buf);
    sprintf(line, "%d %d %d\n", node->ID, node->left_stock, node->price);
    strcat(buf, line);
    collect_show(node->right, buf);
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
            sem_wait(&curr->mutex);
            if (curr->left_stock < amount) {
                Rio_writen(connfd, "Not enough left stocks\n", strlen("Not enough left stocks\n"));
            } else {
                curr->left_stock -= amount;
                Rio_writen(connfd, "[buy] success\n", strlen("[buy] success\n"));
            }
            sem_post(&curr->mutex);
            return;
        }
        curr = (id < curr->ID) ? curr->left : curr->right;
    }
    Rio_writen(connfd, "Invalid stock ID\n", strlen("Invalid stock ID\n"));  // optional
}

void sell_stock(int connfd, int id, int amount) {
    stock_t *curr = root;
    while (curr) {
        if (id == curr->ID) {
            sem_wait(&curr->mutex);
            curr->left_stock += amount;
            Rio_writen(connfd, "[sell] success\n", strlen("[sell] success\n"));
            sem_post(&curr->mutex);
            return;
        }
        curr = (id < curr->ID) ? curr->left : curr->right;
    }
    Rio_writen(connfd, "Invalid stock ID\n", strlen("Invalid stock ID\n"));  // optional
}

void handle_command(int connfd, char *buf) {
    char cmd[16];
    int id, amount;
    char reply[MAXLINE];

    // 클라이언트가 보낸 명령어 자체를 먼저 넣음
    snprintf(reply, sizeof(reply), "%s", buf);  // 명령어 한 줄 복사

    if (sscanf(buf, "%s", cmd) == 1) {
        if (strcmp(cmd, "show") == 0) {
            // show는 여러 줄 결과니까 따로 처리
            char show_buf[MAXLINE] = "";
            collect_show(root, show_buf);
            strcat(reply, show_buf);  // 명령어 + 결과
        } else if (strcmp(cmd, "buy\n") == 0 && sscanf(buf, "%*s %d %d", &id, &amount) == 2) {
            stock_t *curr = root;
            while (curr) {
                if (curr->ID == id) {
                    sem_wait(&curr->mutex);
                    if (curr->left_stock < amount) {
                        strcat(reply, "Not enough left stocks");
                    } else {
                        curr->left_stock -= amount;
                        strcat(reply, "[buy] success/n");
                    }
                    sem_post(&curr->mutex);
                    break;
                }
                curr = (id < curr->ID) ? curr->left : curr->right;
            }
            if (!curr) strcat(reply, "Invalid stock ID");
        } else if (strcmp(cmd, "sell") == 0 && sscanf(buf, "%*s %d %d", &id, &amount) == 2) {
            stock_t *curr = root;
            while (curr) {
                if (curr->ID == id) {
                    sem_wait(&curr->mutex);
                    curr->left_stock += amount;
                    strcat(reply, "[sell] success\n");
                    sem_post(&curr->mutex);
                    break;
                }
                curr = (id < curr->ID) ? curr->left : curr->right;
            }
            if (!curr) strcat(reply, "Invalid stock ID");
        } else {
            strcat(reply, "Unknown command\n");
        }
    }

    Rio_writen(connfd, reply, strlen(reply));
}

void save_stock(FILE *fp, stock_t *node) {
    if (node == NULL) return;
    save_stock(fp, node->left);
    fprintf(fp, "%d %d %d\n", node->ID, node->left_stock, node->price);
    save_stock(fp, node->right);
}

void save_stock_data(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        return;
    }
    save_stock(fp, root);
    fclose(fp);
}

void handle_sigint(int sig) {
    save_stock_data("stock.txt");
    printf("\n[INFO] Server shutting down. stock.txt saved.\n");
    exit(0);
}