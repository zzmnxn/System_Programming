#include "csapp.h"
#include <time.h>
#include <sys/time.h>

#define MAX_CLIENT 100
#define ORDER_PER_CLIENT 5
#define STOCK_NUM 5
#define BUY_SELL_MAX 5
#define SHOW_ONLY_MODE 1 // 1이면 show만, 0이면 buy/sell만 실행

int main(int argc, char **argv)
{
	pid_t pids[MAX_CLIENT];
	int runprocess = 0, status, i;

	int clientfd, num_client;
	char *host, *port, buf[MAXLINE], tmp[3];
	rio_t rio;

	if (argc != 4)
	{
		fprintf(stderr, "usage: %s <host> <port> <client#>\n", argv[0]);
		exit(0);
	}

	host = argv[1];
	port = argv[2];
	num_client = atoi(argv[3]);
	struct timeval start, end;
	gettimeofday(&start, NULL);

	/*	fork for each client process	*/
	while (runprocess < num_client)
	{
		// wait(&state);
		pids[runprocess] = fork();

		if (pids[runprocess] < 0)
			return -1;
		/*	child process		*/
		else if (pids[runprocess] == 0)
		{
			printf("child %ld\n", (long)getpid());

			clientfd = Open_clientfd(host, port);
			Rio_readinitb(&rio, clientfd);
			srand((unsigned int)getpid());

			for (i = 0; i < ORDER_PER_CLIENT; i++)
			{
				if (SHOW_ONLY_MODE)
				{
					strcpy(buf, "show\n");
				}
				else
				{ // buy
					if (rand() % 2 == 0)
					{
						int list_num = rand() % STOCK_NUM + 1;
						int num_to_buy = rand() % BUY_SELL_MAX + 1; // 1~10

						strcpy(buf, "buy ");
						sprintf(tmp, "%d", list_num);
						strcat(buf, tmp);
						strcat(buf, " ");
						sprintf(tmp, "%d", num_to_buy);
						strcat(buf, tmp);
						strcat(buf, "\n");
					}
					// sell
					else
					{
						int list_num = rand() % STOCK_NUM + 1;
						int num_to_sell = rand() % BUY_SELL_MAX + 1; // 1~10

						strcpy(buf, "sell ");
						sprintf(tmp, "%d", list_num);
						strcat(buf, tmp);
						strcat(buf, " ");
						sprintf(tmp, "%d", num_to_sell);
						strcat(buf, tmp);
						strcat(buf, "\n");
					}
					// strcpy(buf, "buy 1 2\n");
				}

				Rio_writen(clientfd, buf, strlen(buf));
				// Rio_readlineb(&rio, buf, MAXLINE);
				printf("[CLIENT %d] Sending: %s", getpid(), buf);
				Rio_readnb(&rio, buf, MAXLINE);
				Fputs(buf, stdout);

				usleep(1000000);
			}

			Close(clientfd);
			exit(0);
		}
		/*	parten process		*/
		/*else{
			for(i=0;i<num_client;i++){
				waitpid(pids[i], &status, 0);
			}
		}*/
		runprocess++;
	}
	for (i = 0; i < num_client; i++)
	{
		waitpid(pids[i], &status, 0);
	}
	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - start.tv_sec) * 1000.0; // sec to ms
	elapsed += (end.tv_usec - start.tv_usec) / 1000.0;	   // us to ms
	printf("Elapsed time: %.2f ms\n", elapsed);

	/*clientfd = Open_clientfd(host, port);
	Rio_readinitb(&rio, clientfd);

	while (Fgets(buf, MAXLINE, stdin) != NULL) {
		Rio_writen(clientfd, buf, strlen(buf));
		Rio_readlineb(&rio, buf, MAXLINE);
		Fputs(buf, stdout);
	}

	Close(clientfd); //line:netp:echoclient:close
	exit(0);*/

	return 0;
}
