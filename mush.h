#ifndef MUSH_H
#define MUSH_H

struct pipe {
	int piperead;
	int pipewrite;
};

char *break_line(char*);

void clear_buf(char*);

void musher(char*);

void sigint_handler();

#endif
