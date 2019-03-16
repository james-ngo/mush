#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "parseline.h"
#define LNMAX 512
#define PROMPT 6
#define NULLERR 21
#define LENERR 39
#define ARGS_MAX 10
#define PIPELINE_MAX 10

struct stage *parseline(char *cmdlne, int *num_stages_ptr) {
	int num, arg_count = 0, cur_stage = 0, in_redir = 0, out_redir = 0;
	int j, pipe_from = 0, i = 0;
	struct stage *stages = (struct stage*)calloc(PIPELINE_MAX,
		sizeof(struct stage));
	char *argv_list[ARGS_MAX];
	char cur_arg[LNMAX];
	char in[LNMAX];
	char out[LNMAX];
	char *marker;
/* We read into a buffer of LNMAX + 1. If we read nothing, we throw an error.
 * If we read LNMAX + 1, we throw an error because the maximum command line
 * length has been exceeded. */
	num = strlen(cmdlne);
	strtok(cmdlne, " ");
/* We replace any space characters in the command line with null characters.
 * This helps us separate this arguments more easily. */
	while (strtok(NULL, " ")) {
		/* twiddle */
	}
	marker = cmdlne;
	strcpy(in, "original stdin");
	strcpy(out, "original stdout");
/* We loop through analyzing each string in the command line. After we
 * scan a particular portion of the command line, we increment i by the length
 * of that portion.*/
	while (i < num + 1) {
/* If we reach a pipe or the end of the line, we a struct stage with all the
 * information about the current stage. We throw an error if we are currently
 * on the 11th stage. We also throw an error if the previous stage indicated
 * output redirection. */
		if (cmdlne[i] == '|' || i >= num) {
			if (cur_stage > 9) {
				fprintf(stderr,"maximum number of commands "
				              "exceeded\n");
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free_all(stages, cur_stage);
				return NULL;			
			}
			if (cmdlne[i] == '|' && !out_redir) {
				sprintf(out, "pipe");
			}
			else if (cmdlne[i] == '|' && out_redir) {
				fprintf(stderr, "%s: ambiguous output\n",
					argv_list[0]);
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free(stages);
				return NULL;
			}
			if (arg_count < 1) {
				if (cmdlne[i] == '|' || cur_stage) {
					write(STDERR_FILENO, "invalid null "
						"command\n", NULLERR);
				}
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free_all(stages, cur_stage);
				return NULL;
			}
			for (j = 0; j < num - 1; j++) {
				if (cmdlne[j] == '\0') {
					cmdlne[j] = ' ';
				}
			}
			cmdlne[i++] = '\0';
			stages[cur_stage].stage = cur_stage;
			stages[cur_stage].pipeline = (char*)malloc(sizeof(char)
				* (strlen(marker) + 1));
			stages[cur_stage].in = (char*)malloc(sizeof(char) *
				(strlen(in) + 1));
			stages[cur_stage].out = (char*)malloc(sizeof(char) *
				(strlen(out) + 1));
			strcpy(stages[cur_stage].pipeline, marker);
			stages[cur_stage].argc = arg_count;
			for (j = 0; j < stages[cur_stage].argc; j++) {
				stages[cur_stage].argv_list[j] = (char*)malloc(
					sizeof(char) * (strlen(
					argv_list[j]) + 1));
				strcpy(stages[cur_stage].argv_list[j],
					argv_list[j]);
				free(argv_list[j]);
			}
			strcpy(stages[cur_stage].in, in);
			strcpy(stages[cur_stage].out, out);
			marker = marker + strlen(marker) + 1;
			sprintf(in, "pipe");
			strcpy(out, "original stdout");
			in_redir = 1;
			pipe_from = 1;
			out_redir = 0;
			arg_count = 0;
			strtok(cmdlne, " ");
			while (strtok(NULL, " ")) {
				/* twiddle */
			}
			cur_stage++;
		}
/* If we reach a '<' character, we throw an error if there the input from this
 * stage was supposed to be piped from the previous stage or if input
 * redirection is attempted twice. We then check if the argument that follows
 * the '<' character is not '<', '>', or '|'. If so, we throw an error. If
 * everything's all good at this point we copy the string into the in variable.
 * */
		else if (cmdlne[i] == '<') {
			if (pipe_from) {
				fprintf(stderr, "%s: ambiguous input\n",
					argv_list[0]);
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free_all(stages, cur_stage);
				return NULL;
			}
			else if (in_redir++ || !arg_count) {
				fprintf(stderr, "%s: bad input redirection\n",
					argv_list[0]);
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free_all(stages, cur_stage);
				return NULL;
			}
			else {
				while (cmdlne[++i] == '\0' ||
					cmdlne[i] == ' ') {
					/* twiddle */
				}
				strcpy(in, cmdlne + i);
				for (j = 0; in[j] != '\0' &&
					in[j] != '<' && in[j] != '>'
					&& in[j] != '|'; j++) {
					/* twiddle */
				}
				if (in[j] == '<' || in[j] == '>'
					|| in[j] == '|' || i >= num) {
					fprintf(stderr,
						"%s: bad input redirection\n",
						argv_list[0]);
					for (j = 0;j < arg_count;j++) {
						free(argv_list[j]);
					}
					free_all(stages, cur_stage);
					return NULL;
				}
				in[j] = '\0';
				i += strlen(in);
			}
		}
/* We do pretty much the exact same thing for '>' as we do for '<'. */
		else if (cmdlne[i] == '>') {
			if (out_redir++ || !arg_count) {
				fprintf(stderr, "%s: bad output redirection\n",
					argv_list[0]);
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				free_all(stages, cur_stage);
				return NULL;
			}
			else {
				while (cmdlne[++i] == '\0' ||
					cmdlne[i] == ' ') {
					/* twiddle */
				}
				strcpy(out, cmdlne + i);
				for (j = 0; out[j] != '\0' &&
					out[j] != '<' && out[j] != '>'
					&& out[j] != '|'; j++) {
					/* twiddle */
				}
				if (out[j] == '<' || out[j] == '>' ||
					out[j] == '|' || i >= num) {
					fprintf(stderr,
						"%s: bad output redirection\n",
						argv_list[0]);
					for (j = 0;j < arg_count;j++) {
						free(argv_list[j]);
					}
					free_all(stages, cur_stage);
					return NULL;
				}
				out[j] = '\0';
				i += strlen(out);
			}
		}
/* If we reach space of null characters we simply scan past them. */
		else if (cmdlne[i] == '\0' || cmdlne[i] == ' ') {
			i++;
		}
/* This else block assumes we have reached an additional argument to a command
 * or are at a new command. Either way, we check to see if we have exceeded the
 * 10 argument limit. If not, we add this string to a argv_list.*/
		else {
			if (arg_count == ARGS_MAX) {
				fprintf(stderr, "maximum pipeline argument "
					        "length exceeded\n");
				for (j = 0; j < arg_count; j++) {
					free(argv_list[j]);
				}
				return NULL;
			}
			while (cmdlne[i] == '\0') {
				i++;
			}
			strcpy(cur_arg, cmdlne + i);
			for (j = 0; cur_arg[j] != '\0' && cur_arg[j] != '<' &&
				cur_arg[j] != '>' && cur_arg[j] != '|' &&
				cur_arg[j] != ' '; j++) {
				/* twiddle */
			}
			cur_arg[j] = '\0';
			argv_list[arg_count] = (char*)malloc(sizeof(char)
				* (strlen(cur_arg) + 1));
			strcpy(argv_list[arg_count], cur_arg);
			i += strlen(cur_arg);
			arg_count++;
		}
	}
/* Don't forget to free in, out, and the entire argv_list. */
	*num_stages_ptr = cur_stage;
	return stages;
}

/* This function does as the name implies.*/
void free_all(struct stage *stages, int num_stages) {
	int i, j;
	for (i = 0; i < num_stages; i++ ) {
		free(stages[i].pipeline);
		free(stages[i].in);
		free(stages[i].out);
		for (j = 0; j < stages[i].argc; j++) {
			free(stages[i].argv_list[j]);
		}
	}
	free(stages);
}

/* This does all the output to stdout. */
void print_stage(struct stage *stages, int num_stages) {
	int i, j;
	for (i = 0; i < num_stages; i++) {
		printf("\n--------\n"
		       "Stage %d: \"%s\"\n"
       	               "--------\n", stages[i].stage, stages[i].pipeline);
		printf("     input: %s\n", stages[i].in);
		printf("    output: %s\n", stages[i].out);
		printf("      argc: %d\n", stages[i].argc);
		printf("      argv: ");
		free(stages[i].pipeline);
		free(stages[i].in);
		free(stages[i].out);
		for (j = 0; j < stages[i].argc; j++) {
			printf("\"%s\"", stages[i].argv_list[j]);
			free(stages[i].argv_list[j]);
			if (j != stages[i].argc - 1) {
				printf(",");
			}
			else {
				printf("\n");
			}
		}
	}	
}
