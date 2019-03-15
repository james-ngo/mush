#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "parseline.h"
#include "mush.h"
#define DISK 4096
#define LNMAX 512
#define PIPELNE_MAX 10
#define PROMPT 4

/* This global variable helps with displaying the prompt in the event that a
 * signal is received. */

int sig_received = 0;

int main(int argc, char *argv[]) {
	int fdin;
	char buf[DISK];
	struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);
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
	buf[0] = '\0';
/* We take our input and do practically all the work through the 'musher'
 * function. We also reprompt if the input is from stdin and we have not yet
 * received a signal. */
	while (read(fdin, buf, DISK)) {
		musher(buf);
		clear_buf(buf);
		if (fdin == STDIN_FILENO) {
			if (sig_received) {
				write(STDOUT_FILENO, "\n", 1);
			}
			write(STDOUT_FILENO, "8-P ", PROMPT);
		}
		sig_received = 0;
	}
	/*
	if (fdin == STDIN_FILENO) {
		printf("\n");
	}
	*/
	return 0;
}

/* We clear the input buffer after every musher call. */
void clear_buf(char *buf) {
	int i;
	for (i = 0; i < DISK; i++) {
		buf[i] = '\0';
	}
}

/* This does pretty much all the work. It parses input, breaking it into
 * separate lines and recursively calling itself with the next line as its
 * input. */
void musher(char *buf) {
	char *cur_line, *next_line;
	struct stage *stages;
	int i, j, status, num_stages, fdin = STDIN_FILENO;
	int fdout = STDOUT_FILENO;
	pid_t child;
	struct pipe *pipes;
/* If the buffer begins with a null character or newline charcters. We have
 * nothing left to execute. */
	if (!*buf || *buf == '\n') {
		return;
	}
/* The next two lines are for when we receive the commands from an input file.
 */
	cur_line = buf;
	next_line = break_line(buf);
/* Here, we parse the line into struct stages and that. */
	if (NULL == (stages = parseline(cur_line, &num_stages))) {
		return;
	}
	pipes = (struct pipe*)malloc(sizeof(struct pipe) * (num_stages - 1));
	for (i = 0; i < num_stages - 1; i++) {
		pipe((int*)(pipes + i));
	}
	for (i = 0; i < num_stages; i++) {
		if (!strcmp(stages[i].argv_list[0], "cd")) {
			chdir(stages[i].argv_list[1]);
		}
		else if ((child = fork()) == 0) {
			/* child */
			if (!strcmp(stages[i].in, "pipe")) {
				/* pipe from previous stage */
				dup2(pipes[i - 1].piperead, STDIN_FILENO);
			}
			else if (strcmp(stages[i].in, "original stdin")) {
				if (-1 == (fdin = open(stages[i].in,
					O_RDONLY))) {
					perror(stages[i].in);
					return;
				}
				dup2(fdin, STDIN_FILENO);
				close(fdin);
			}
			if (!strcmp(stages[i].out, "pipe")) {
				/* pipe to next stage */
				dup2(pipes[i].pipewrite, STDOUT_FILENO);
			}
			else if (strcmp(stages[i].out, "original stdout")) {
				if (-1 == (fdout = creat(stages[i].out,
					S_IRUSR | S_IWUSR))) {
					perror(stages[i].out);
					exit(3);
				}
				dup2(fdout, STDOUT_FILENO);
				close(fdout);
			}
			for (j = 0; j < num_stages - 1; j++) {
				close(pipes[j].piperead);
				close(pipes[j].pipewrite);
			}
			execvp(stages[i].argv_list[0], stages[i].argv_list);
			perror(stages[i].argv_list[0]);
			return;
		}
		for (j = 0; j < stages[i].argc; j++) {
			free(stages[i].argv_list[j]);
		}
		free(stages[i].pipeline);
		free(stages[i].in);
		free(stages[i].out);
	}
	/* parent */
	if (-1 == child) {
		perror("fork");
		exit(3);
	}
	for (j = 0; j < num_stages - 1; j++) {
		close(pipes[j].piperead);
		close(pipes[j].pipewrite);
	}
	while (wait(&status) > 0) {
		/* do nothing */
	}
	if (errno != ECHILD && errno != EINTR) {
		perror("wait");
		exit(2);
	}
	free(pipes);
	free(stages);
	musher(next_line);
}

void sigint_handler() {
	sig_received = 1;
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
