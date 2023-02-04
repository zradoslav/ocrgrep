#ifndef PAGE_H
#define PAGE_H

typedef struct {
	int width;
	int height;
	int bytes_per_pixel;
	int bytes_per_line;
	unsigned char* data;
	char* text;
	int textlen;
} Page;

#endif
