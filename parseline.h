#ifndef PARSELINE_H
#define PARSELINE_H

struct stage {
	int stage;
	char *pipeline;
	int argc;
	char *in;
	char *out;
	char *argv_list[10];
};

void print_stage(struct stage*, int);

void free_all(struct stage*, int);

#endif
