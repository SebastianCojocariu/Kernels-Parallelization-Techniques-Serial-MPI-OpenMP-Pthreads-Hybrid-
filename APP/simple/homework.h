#ifndef HOMEWORK_H
#define HOMEWORK_H
#define MAX 10000

char type;
int heigth;
int width;
int maxval;

typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;

}color;

typedef struct{
	unsigned char white_black;
}black;


typedef struct {
	void **matrix;
}image;

typedef struct{
	int index;
	image *img_before;
	image *img_after;
	int num_filters;
	char **filters;
}argumente;

void readInput(const char * fileName, image *img);

void writeData(const char * fileName, image *img);

void resize(image *in, image * out);

#endif /* HOMEWORK_H */