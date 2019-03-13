#ifndef PARSELINE_H
#define PARSELINE_H
#define ARGS_MAX 10

struct stage {
	int stage;
	char *pipeline;
	int argc;
	char *in;
	char *out;
	char *argv_list[ARGS_MAX];
};

struct stage *parseline(char*, int*);

void print_stage(struct stage*, int);

void free_all(struct stage*, int);

#endif
