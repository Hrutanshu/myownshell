//to exit the shell type exit and press enter or ctrl+z
#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h>
#include<fcntl.h> 
#include<errno.h> 
#define MAX 1000 //max string length
#define MAXCOM 100 //max commands
void sigintHandler(int sig_num) 
{ 
    signal(SIGINT, sigintHandler);
    fflush(stdout);
} 
void seperatecommandnarguments(char* str, char** str2) {
	int i, j = 0, k = 0;
	char *token;
	token = (char *)malloc(1000);
	for(i = 0; i < strlen(str); i++) {
		if((str[i] == ' ') || (str[i] == '|') || (str[i] == '>') || (str[i] == '<')) {
			if((str[i] == ' ') && (j == 0)) {
				continue;
			}
			else if(str[i] == ' ') {
				token[j] = '\0';
				j = 0;
				str2[k] = (char *)malloc(strlen(token));
				strcpy(str2[k++], token);
				free(token);
				token = (char *)malloc(1000);
			}
			else {
				if(j) {
					token[j] = '\0';
					j = 0;
					str2[k] = (char *)malloc(strlen(token));
					strcpy(str2[k++], token);
					free(token);
					token = (char *)malloc(1000);
				}
				token[j++] = str[i];
				token[j] = '\0';
				j = 0;
				str2[k] = (char *)malloc(strlen(token));
				strcpy(str2[k++], token);
				free(token);
				token = (char *)malloc(1000);
			}
		}
		else {
			token[j++] = str[i];
		}
	}
	if(j) {
		j = 0;
		str2[k] = (char *)malloc(strlen(token));
		strcpy(str2[k++], token);
		free(token);
		token = (char *)malloc(1000);
	}
	str2[k] = NULL;
}

int nofpipes(char *str, int *noofpipe) {
	int i;
	for(i = 0; i < strlen(str); i++) {
		if(str[i] == '|')
			(*noofpipe)++;
	}
}

int cdnexit(char** check) {
	int i, index = 0; 
	char* cmds[2];
	char path[1000];
	char cwd[1000];
	cmds[0] = "exit"; 
	cmds[1] = "cd";
	for (i = 0; i < 2; i++) { 
		if (strcmp(check[0], cmds[i]) == 0) { 
			index = i + 1; 
			break; 
		} 
	} 
	switch (index) { 
	case 1: 
		exit(0); 
	case 2:
		if(!check[1]) {
			chdir(getenv("HOME"));
			return 1;
		}
    	strcpy(path,check[1]);
		if(check[1][0] != '/') {
			getcwd(cwd,sizeof(cwd));
			strcat(cwd,"/");
			strcat(cwd,path);
			chdir(cwd);
		}
		else {
			chdir(check[1]);
		}
		return 1;  
	default: 
		break; 
	} 
	return 0;
}
int main() {
	
	int flag = 0, i = 0, n, noofpipe = 0, j, k;
	char str[MAX], *strwopipe[MAXCOM], *token, *token1, ch;
	printf("\033[H\033[J");
	char cur_dir[1024];
	while(1) {
		signal(SIGINT, sigintHandler);
		noofpipe = 0;
		getcwd(cur_dir, sizeof(cur_dir)); //get current working directory
		printf("Dir: %s", cur_dir);
		printf(">>");
		i = 0;
		ch = getchar();
		while(ch != '\n') {
			str[i++] = ch;
			ch = getchar();
		}
		str[i] = '\0';
		if(strlen(str) == 0) {
			continue;
		}
		nofpipes(str, &noofpipe);//get the number of pipes
		if(noofpipe) {
			char *partsofpipe[noofpipe + 1];
			char *strwpipe[noofpipe + 1][MAXCOM];
			token = strtok(str, "|");
			partsofpipe[0] = (char *)malloc(strlen(token));
			strcpy(partsofpipe[0], token);
			seperatecommandnarguments(partsofpipe[0], strwpipe[0]);
			token = strtok(NULL, "|");
			i = 1;
			while(token) {	
				partsofpipe[i] = (char *)malloc(strlen(token));
				strcpy(partsofpipe[i], token);
				seperatecommandnarguments(partsofpipe[i], strwpipe[i]);
				token = strtok(NULL, "|");
				i++;
			}
			int pipefd[2], p1, p2 = 0;
			for(j = 0; j <= noofpipe; j++) {
				if (pipe(pipefd) < 0) { 
					printf("\nPipe could not be initialized\n"); 
					return 0;
				}
				p1 = fork(); 
				if (p1 < 0) { 
					printf("\nCould not fork\n"); 
					return 0;
				}
				if (p1 == 0) { 
					dup2(p2, 0);
					if(j != noofpipe)
						dup2(pipefd[1], 1);
					close(pipefd[0]);
					char *ip, *op;
					ip = NULL;
					op = NULL;
					int ff = 1, fdip, fdop;
					for(i = 0; strwpipe[j][i]; i++) {
						if(strcmp(strwpipe[j][i], ">") == 0) {
							op = strwpipe[j][i + 1];
							fdop = open(op, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							close(fdop);
							if(ff) {
								ff = 0;
								strwpipe[j][i] = NULL;
							}
						}
						else if(strcmp(strwpipe[j][i], "<") == 0) {
							ip = strwpipe[j][i + 1];
							fdip = open(ip, O_RDONLY);
							if(fdip == -1) {
								perror("Cannot open input file\n");
								exit(1);
							}
							close(fdip);
							if(ff) {
								ff = 0;
								strwpipe[j][i] = NULL;
							}
						}
					}
					if(ip) {
						close(0);
						fdip = open(ip, O_RDONLY);
						if(fdip == -1) {
							perror("Cannot open input file\n");
							exit(1);
						}
					}
					if(op) {
						close(1);
						fdop = open(op, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
						if(fdop == -1) {
							perror("Cannot open output file\n");
							exit(1);
						}
					}
					if (execvp(strwpipe[j][0], strwpipe[j]) < 0) { 
						printf("\nCould not execute command 1..\n"); 
						exit(0); 
					}
					if(ip)
						close(fdip);
					if(op)
						close(fdop);
					exit(1);
				}
				else {
					wait(NULL);
					close(pipefd[1]);
					p2 = pipefd[0];
				}
			}

		}
		else {
			i = 0;
			seperatecommandnarguments(str, strwopipe);
			if (cdnexit(strwopipe)) {
				continue;
			}
			else {
				int pid = fork(); 
				if (pid == -1) { 
					printf("\nFailed forking child..\n"); 
				} 
				else if (pid == 0) {
					char *ip, *op;
					ip = NULL;
					op = NULL;
					int ff = 1, fdip, fdop;
					for(i = 0; strwopipe[i]; i++) {
						if(strcmp(strwopipe[i], ">") == 0) {
							op = strwopipe[i + 1];
							fdop = open(op, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							close(fdop);
							if(ff) {
								ff = 0;
								strwopipe[i] = NULL;
							}
						}
						else if(strcmp(strwopipe[i], "<") == 0) {
							ip = strwopipe[i + 1];
							fdip = open(ip, O_RDONLY);
							if(fdip == -1) {
								perror("Cannot open input file\n");
								exit(1);
							}
							close(fdip);
							if(ff) {
								ff = 0;
								strwopipe[i] = NULL;
							}
						}
					}
					if(ip) {
						close(0);
						fdip = open(ip, O_RDONLY);
						if(fdip == -1) {
							perror("Cannot open input file\n");
							exit(1);
						}
					}
					if(op) {
						close(1);
						fdop = open(op, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
						if(fdop == -1) {
							perror("Cannot open output file\n");
							exit(1);
						}
					}
					if (execvp(strwopipe[0], strwopipe) < 0) {
						printf("\nCould not execute command..\n"); 
					}
					if(ip)
						close(fdip);
					if(op)
						close(fdop);
					exit(0); 
				}
				else { 
					wait(NULL); 
				} 
			}
		}
	}
	return 0;
}
