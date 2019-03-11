#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "parseline.h"
#include "mush.h"
#define DISK 4096
#define LNMAX 512
#define PROMPT 4

int main(int argc, char *argv[]) {
	int fdin, num;
	char buf[DISK];
	if (argc == 1) {
		fdin = STDIN_FILENO;
	}
	else if (argc > 2) {
		fprintf(stderr, "usage: %s [ scriptfile ]\n", argv[0]);
	}
	else {
		if (-1 == (fdin = open(argv[1], O_RDONLY))) {
			perror(argv[1]);
			exit(3);
		}
	}
	if (fdin == STDIN_FILENO) {
		write(STDOUT_FILENO, "8-P ", PROMPT);
	}
	while ((num = read(fdin, buf, DISK)) > 0) {
		musher(buf);
		clear_buf(buf);
		if (fdin == STDIN_FILENO) {
			write(STDOUT_FILENO, "8-P ", PROMPT);
		}
	}
	printf("\n");
	return 0;
}

void clear_buf(char *buf) {
	int i;
	for (i = 0; i < DISK; i++) {
		buf[i] = '\0';
	}
}

void musher(char *buf) {
	char *cur_line, *next_line;
	struct stage *stages;
	int i, j, status, num_stages;
	int fd[2] = { STDIN_FILENO, STDOUT_FILENO };
	pid_t child;
	if (!*buf) {
		return;
	}
	cur_line = buf;
	next_line = break_line(buf);
	stages = parseline(cur_line, &num_stages);
	for (i = 0; i < num_stages; i++) {
		if (strcmp(stages[i].in, "original stdin")) {
			if (!strcmp(stages[i].in, "pipe")) {
				/* pipe from previosu stage */
			}
			else {
				if (-1 == (fd[0] = open(stages[i].in,
					O_RDONLY))) {
					perror(stages[i].in);
					exit(3);
				}
				
			}
		}
		if (strcmp(stages[i].out, "original stdout")) {
			if (!strcmp(stages[i].out, "pipe")) {
				pipe(fd);
			}
			else {
				if (-1 == (fd[1] = creat(stages[i].out,
					S_IRWXU))) {
					perror(stages[i].out);
					exit(3);
				}
			}
		}
		if (!strcmp(stages[i].argv_list[0], "cd")) {
			chdir(stages[i].argv_list[1]);
		}
		else if ((child = fork())) {
			/* parent */
			if (-1 == child) {
				perror("fork");
				exit(3);
			}
			if (-1 == wait(&status)) {
				perror("wait");
				exit(4);
			}
			if (!WIFEXITED(status) || WEXITSTATUS(status)) {
				exit(EXIT_FAILURE);
			}
		}
		else {
			/* child */
			if (-1 == dup2(fd[0], STDIN_FILENO)) {
				perror("dup2");
				exit(5);
			}
			if (-1 == dup2(fd[1], STDOUT_FILENO)) {
				perror("dup2");
				exit(6);
			}
			if (fd[0] != STDIN_FILENO) {
				printf("close\n");
				close(fd[0]);
			}
			if (fd[1] != STDOUT_FILENO) {
				printf("close\n");
				close(fd[1]);
			}
			execvp(stages[i].argv_list[0], stages[i].argv_list);
			perror(stages[i].argv_list[0]);
			exit(3);
		}
		for (j = 0; j < stages[i].argc; j++) {
			free(stages[i].argv_list[j]);
		}
		free(stages[i].pipeline);
		free(stages[i].in);
		free(stages[i].out);
		free(stages);
	}
	musher(next_line);
}

char *break_line(char *cmd) {
	char *ptr;
	ptr = cmd;
	for (; *ptr != '\n'; ptr++) {
		/* do nothing */
	}
	*ptr = '\0';
	return ptr + 1;
}
