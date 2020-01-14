#include "homework.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int num_threads = 4;
pthread_barrier_t barrier;

//functie care genereaza matricea de pixeli corespunzatoare parametrilor primiti.
//type = '5' matrice alb-negru,type = '6' matrice color
void **generate_image(int heigth,int width,char type){

	int i,j;

	if(type == '5'){	
		black **aux = (black**)malloc(heigth*sizeof(black*));
		for(i = 0;i < heigth; i++){
			aux[i] = (black*)malloc(width*sizeof(black));
		}

		return (void**)aux;
	}

	else if(type == '6'){
		
		color **aux = (color**)malloc(heigth*sizeof(color*));
		if(aux == NULL){
			exit(-1);
		}

		for(i = 0;i < heigth; i++){
			aux[i] = (color*)malloc(width*sizeof(color));
			if(aux[i] == NULL){
				exit(-1);
			}
		}
		return (void**)aux;
	}
	
	//nu se ajunge aici daca se da un parametru bun ('5' sau '6')
	return NULL;
}


void readInput(const char * fileName, image *img) {
	FILE *in = fopen(fileName,"r");
	char byte;
	
	//actualizam tipul
	int i = 0;
	char *buffer = malloc(10);
	fscanf(in,"%s\n", buffer);
	type = buffer[1];
	fscanf(in,"%d\n%d\n%d\n", &width, &heigth, &maxval);
	free(buffer);

	//citire matrice de pixeli
	
	//daca avem poza alb-negru
	if(type == '5'){
		int i,j;

		black **aux = (black**)generate_image(heigth,width,type);
		char *buffer = malloc(heigth*width*sizeof(char));
		//citire.Facem intr-un buffer cu fread deoarece folosing fscanf direct pe matrice duce la delay prea mare.
		fread(buffer,sizeof(char),heigth*width,in);

		//transferam din buffer in matricea de pixeli aux.
		for(i = 0;i < heigth; i++){
			for(j = 0;j < width; j++){
				aux[i][j].white_black = buffer[i*width+j];
			}
		}

		img->matrix = aux;
		//dealocam buffer
		free(buffer);

	}

	//daca avem poza color
	if(type == '6'){

		int i,j;
		
		color **aux = (color**)generate_image(heigth,width,type);
		char *buffer = malloc(heigth*width*3*sizeof(char));
		//citire.Facem intr-un buffer cu fread deoarece folosing fscanf direct pe matrice duce la delay prea mare.
		fread(buffer,sizeof(char),3*heigth*width,in);

		//transferam din buffer in matricea de pixeli aux.
		for(i = 0;i < heigth; i++){
			for(j = 0;j < width; j++){
				aux[i][j].r = buffer[3*(i*width+j)];
				aux[i][j].g = buffer[3*(i*width+j)+1];
				aux[i][j].b = buffer[3*(i*width+j)+2];
			}
		}

		img->matrix = aux;
		//dealocam buffer
		free(buffer);

	}

	fclose(in);



}

void writeData(const char * fileName, image *img) {

	
	int i,j;

	FILE *out = fopen(fileName,"wr");

	char header[100];
	sprintf(header,"P%c\n%d %d\n255\n",type,width,heigth);
	fwrite(header,1,strlen(header),out);



	if(type == '5'){
		//buffer in care vom pune informatiile pentru a le transfera in fisier cu fread.
		char *buffer = malloc(sizeof(char) * heigth * width);
		//transferul din matricea de pixeli in buffer
		black **aux = (black**)img->matrix;
		for(i = 0;i < heigth;i++)
			for(j = 0;j < width;j++){
				buffer[i*width + j] = aux[i][j].white_black;
			}
		//scriere din buffer in fisierul de iesire.
		fwrite(buffer,sizeof(char),heigth * width,out);
		free(buffer);
	}
	else if(type == '6'){
		//buffer in care vom pune informatiile pentru a le transfera in fisier cu fread.
		char *buffer = malloc(3*sizeof(char) * heigth * width);
		//transferul din matricea de pixeli in buffer
		color **aux = (color**)img->matrix;

		for(i = 0;i < heigth; i++){
			for(j = 0;j < width; j++){
				buffer[3*(i * width + j)] = aux[i][j].r ;
				buffer[3*(i * width + j)+1] = aux[i][j].g ;
				buffer[3*(i * width + j)+2] = aux[i][j].b ;
			}
		}
		//scriere din buffer in fisierul de iesire.
		fwrite(buffer,sizeof(char),3 * heigth * width,out);
		free(buffer);	
	}
	fclose(out);


}


float **makeMatrix(float v[3][3]){
	int i,j;
	float **res = (float**)malloc(3*sizeof(float*));
	for(i = 0;i < 3; i++)
		res[i] = (float*)malloc(3*sizeof(float));
	for(i = 0;i < 3; i++)
		for(j = 0;j < 3;j++)
			res[i][j] = v[i][j];
	return res;

}

void *f_middle(void* arg){

	int index = ((argumente*)arg)->index;
	image *before = ((argumente*)arg)->img_before;
	image *after = ((argumente*)arg)->img_after;
	char **filters = ((argumente*)arg)->filters;
	int num_filters = ((argumente*)arg)->num_filters;
	//calculam linia de start si linia de finish pe care o are de executat acest thread
	int start = (index == 0)?1:index * (heigth/num_threads);
	int end = (index == num_threads - 1)?heigth - 2:(index+1)*(heigth/num_threads)-1;
	
	float **v;
	
	for(int k = 0; k < num_filters; k++){
		
		char *filter = filters[k];
		//printf("%s\n",filter);
		
		if(strcmp(filter,"smooth") == 0){
			float aux[3][3] = {{1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f},{1.0f,1.0f,1.0f}};
			v = makeMatrix(aux);
		}
		else if(strcmp(filter,"blur") == 0){
			float aux[3][3] = {{1.0f,2.0f,1.0f},{2.0f,4.0f,2.0f},{1.0f,2.0f,1.0f}};
			v = makeMatrix(aux);
		}
		else if(strcmp(filter,"sharpen") == 0){
			float aux[3][3] = {{0.0f,-2.0f,0.0f},{-2.0f,11.0f,-2.0f},{0.0f,-2.0f,0.0f}};
			v = makeMatrix(aux);
		}
		else if(strcmp(filter,"mean") == 0){
			float aux[3][3] = {{-1.0f,-1.0f,-1.0f},{-1.0f,9.0f,-1.0f},{-1.0f,-1.0f,-1.0f}};
			v = makeMatrix(aux);

		}
		else if(strcmp(filter,"emboss") == 0){
			float aux[3][3] = {{0.0f,1.0f,0.0f},{0.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f}};
			v = makeMatrix(aux);
			
		}
		else if(strcmp(filter,"normal") == 0){
			float aux[3][3] = {{0.0f,0.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,0.0f,0.0f}};
			v = makeMatrix(aux);
		}
		else{
			return NULL;
		}

		float pondere = 0;

		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 3; j++)
				pondere += v[i][j];

		if(strcmp(filter,"emboss") == 0)
			pondere = 1;	

		if(type == '5'){
			black **dest_image = (black**)after->matrix;
			black **source_image = (black**)before->matrix;
			
			for(int i = start; i <= end ;i++){
				
				for(int j = 1;j < width-1;j++){
					float sum = 0;
					for(int k = -1; k <= 1; k++){
						for(int l =-1 ;l <= 1; l++){
							sum += source_image[i+k][j+l].white_black * (v[1+k][1+l]/pondere);
						}
					}
					dest_image[i][j].white_black = (unsigned char)(sum);	
						
				}
			}
				
		}

		if(type == '6'){			
			color **dest_image = (color**)after->matrix;
			color **source_image = (color**)before->matrix;

			for(int i = start; i <= end ;i++){
				for(int j = 1;j < width-1;j++){
					float sum_r = 0;
					float sum_g = 0;
					float sum_b = 0;

					for(int k = -1; k <= 1; k++){
						for(int l =-1 ;l <= 1; l++){
							sum_r += source_image[i+k][j+l].r * (v[1+k][1+l]/pondere);
							sum_g += source_image[i+k][j+l].g * (v[1+k][1+l]/pondere);
							sum_b += source_image[i+k][j+l].b * (v[1+k][1+l]/pondere);
						}
					}

					dest_image[i][j].r = (unsigned char)(sum_r);
					dest_image[i][j].g = (unsigned char)(sum_g);
					dest_image[i][j].b = (unsigned char)(sum_b);
					
						
				}
			}
				
		}


		pthread_barrier_wait(&barrier);
		if(index == 0){
			void **aux = before->matrix;
			before->matrix = after->matrix;
			after->matrix = aux;
		}
		pthread_barrier_wait(&barrier);
	}



	return NULL;
}


int main(int argc, char * argv[]) {
	int i,j;
	int current_index = 0;
	int num_filters = argc - 3;
	image *img = (image*)malloc(2 * sizeof(image));
	readInput(argv[1],&img[0]);
	readInput(argv[1],&img[1]);

	pthread_barrier_init(&barrier, NULL, num_threads);

	pthread_t tid[num_threads];
	int thread_id[num_threads];

	//creem argumentele pentru functia f_middle
	argumente v[num_threads];

	printf("num_threads = %d, num_filters = %d\n",num_threads, num_filters);
	for(i = 0;i < num_threads; i++){
		v[i].index = i;
		v[i].img_before = &img[0];
		v[i].img_after = &img[1];

		char **filters = (char**)malloc(num_filters * sizeof(char*));
		for(int j = 0; j < num_filters; j++){
			filters[j] = (char*)malloc(100 * sizeof(char));
			strcpy(filters[j],argv[j + 3]);
		}
		v[i].filters = filters;
		
		v[i].num_filters = num_filters;
	}

	for(i = 0;i < num_threads; i++)
		thread_id[i] = i;

	for(i = 0; i < num_threads; i++) {
		pthread_create(&(tid[i]), NULL, f_middle,&(v[i]));
	}

	for(i = 0; i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}


	writeData(argv[2],&img[0]);

}



