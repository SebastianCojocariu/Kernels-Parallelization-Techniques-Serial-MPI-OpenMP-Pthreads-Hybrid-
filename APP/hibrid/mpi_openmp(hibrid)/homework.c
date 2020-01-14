#include <time.h>
#include <unistd.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

int num_threads = 4;

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
	char type;
	int heigth;
	int width;
	int maxval;
}image;

int rank;


typedef struct{
	int index;
	int start;
	int end;
	image *img_after;
	image *img_before;
	char *filter;
}argumente;


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
		for(i = 0;i < heigth; i++){
			aux[i] = (color*)malloc(width*sizeof(color));
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
	fscanf(in, "%s\n",buffer);
	img->type = buffer[1];
	fscanf(in,"%d\n%d\n%d\n",&img->width,&img->heigth,&img->maxval);
	
	//citire matrice de pixeli

	char type = img->type;
	int width = img->width;
	int heigth = img->heigth;
	
	//daca avem poza alb-negru
	if(type == '5'){
		int i,j;
		black **aux = (black**)generate_image(heigth,width,type);
		buffer = malloc(heigth*width*sizeof(char));
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

//char *filter,image *img_before, image *img_after, int l1, int l2

void applyFilter(image *img_before, image *img_after, char *filter, int start, int end){
	int l1 = start;
	int l2 = end;

	int i, j, k, l;
	int heigth = img_before->heigth;
	int width = img_before->width;
	float **v;

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
		return;
	}


	float pondere = 0;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			pondere += v[i][j];
	if(strcmp(filter,"emboss") == 0)
		pondere = 1;
	
	if(img_before->type == '5'){
		black **before = (black**)img_before->matrix;
		black **after = (black**)img_after->matrix;
		float sum;
		#pragma omp parallel shared(before, after, v, pondere)
		{
			#pragma omp for private(i, j, k ,l, sum)
			for(i = l1; i < l2;i++){
				for(j = 1; j < width - 1; j++){
					sum = 0;
					for(k = -1; k <= 1; k++){
						for(l =-1 ; l <= 1; l++){
							sum += before[i + k][j + l].white_black * (v[1 + k][1 + l]/pondere);

						
						}
					}
					after[i][j].white_black = (unsigned char)(sum);				
				}
			}
		}
	}
	if(img_before->type == '6'){
		color **before = (color**)img_before->matrix;
		color **after = (color**)img_after->matrix;
		float sum_r, sum_g, sum_b;
		#pragma omp parallel shared(before, after, v, pondere)
		{
			#pragma omp for private(i, j, k ,l, sum_r, sum_g, sum_b)
			for(i = l1; i < l2;i++){
				for(j = 1;j < width-1;j++){
					sum_r = 0;
					sum_g = 0;
					sum_b = 0;

					for(int k = -1; k <= 1; k++){
						for(int l =-1 ;l <= 1; l++){
							sum_r += before[i+k][j+l].r * (v[1+k][1+l]/pondere);
							sum_g += before[i+k][j+l].g * (v[1+k][1+l]/pondere);
							sum_b += before[i+k][j+l].b * (v[1+k][1+l]/pondere);
						}
					}
					after[i][j].r = (unsigned char)(sum_r);
					after[i][j].g = (unsigned char)(sum_g);
					after[i][j].b = (unsigned char)(sum_b);	
						
				}
			}
		}
	}

	void **aux = img_before->matrix;
	img_before->matrix = img_after->matrix;
	img_after->matrix = aux;

	return;

}


unsigned char *preparePackage(image *img,int l1,int l2){
	char type = img->type;
	int width = img->width;
	unsigned char *buffer;
	int i,j;
	if(type == '5'){
		buffer = (unsigned char*)malloc(sizeof(unsigned char)*(l2-l1)*width);
		black **aux = (black**)img->matrix;
		for(i = l1;i < l2;i++){
			for(j = 0;j < width;j++){
				buffer[(i - l1) * width + j] = aux[i][j].white_black;
			}
		}
	
	}
	else if(type == '6'){

		buffer = (unsigned char*)malloc(3*sizeof(unsigned char)*(l2-l1)*width);
		color **aux = (color**)img->matrix;

		for(i = l1;i < l2;i++){
			for(j = 0;j < width;j++){
				buffer[3*((i-l1)*width+j)] = aux[i][j].r ;
				buffer[3*((i-l1)*width+j)+1] = aux[i][j].g ;
				buffer[3*((i-l1)*width+j)+2] = aux[i][j].b ;
			}
		}	
	}

	return buffer;
}


void transfer_to_matrix(image *img,unsigned char *buffer,int line){
	char type = img->type;
	if(type == '5'){
			black **aux = (black**)img->matrix;
			for(int i = 0; i < img->width; i++)
				aux[line][i].white_black = buffer[i];
		}
		else if(type == '6'){
			color **aux = (color**)img->matrix;
			for(int i = 0;i < img->width; i++){
				aux[line][i].r = buffer[3 * i];
				aux[line][i].g = buffer[3 * i + 1];
				aux[line][i].b = buffer[3 * i + 2];
				}

		}
}


int main(int argc, char * argv[]) {
	int i,j;
    int nProcesses;
    int heigth,width;
    char type;
    int no_filters = argc - 3;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
 
	

	if(rank == 0){
		image input;
		//citeste imaginea
		readInput(argv[1], &input);
		//-----------------------

		//info
		heigth = input.heigth;
		width = input.width;
		type = input.type;

		//-----------------------
		if(nProcesses <= 1){
			printf("there must be at least 2 processes\n");
			return 0;
		}
		
		MPI_Bcast(&heigth,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&width,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&type,1,MPI_CHAR,0, MPI_COMM_WORLD);
		

		//pregatim bufferele in care vom primi informatiile de la procese
		unsigned char **buffer_list = (unsigned char**)malloc(nProcesses*sizeof(unsigned char*));
		int num_workers = nProcesses - 1;
		int dimensions[nProcesses];
		int start_send_list[nProcesses];
		int end_send_list[nProcesses];
		int start_recv_list[nProcesses];
		int end_recv_list[nProcesses];
		int dimensiune_line = (type == '5')?width:3 * width;
		int max_dimension = -1;

		//trimitem pentru fiecare process

		for(i = 1; i < nProcesses;i++){
			
			//calculam cate linii trimitem pentru fiecare

			int start = (i == 1)?0: (i-1) * (heigth/num_workers) - 1;
			int end = (i == num_workers)?heigth - 1: i * (heigth/num_workers);

			start_send_list[i] = start;
			end_send_list[i] = end;

			start_recv_list[i] = (i == 1)?start:start + 1;
			end_recv_list[i] = (i == num_workers)?end:end-1;
			//--------------------------------------------

			dimensions[i] = (end - start + 1) * width;
			dimensions[i] = (type == '5')?dimensions[i]:3 * dimensions[i];
			dimensiune_line = dimensions[i] / (end - start + 1);
			if(max_dimension < dimensions[i])
				max_dimension = dimensions[i];

			
			unsigned char *buffer;
			int dimension = dimensions[i];

			unsigned char *buff = preparePackage(&input, start, end + 1);

			//trimite catre fiecare proces
			MPI_Ssend(buff, dimension, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
			free(buff);
		}

		
		
		int cnt = 2 * num_workers - 2;
		
		unsigned char ***auxiliary_buffs = (unsigned char***)malloc(nProcesses * sizeof(unsigned char **));
		for(int i = 0; i < nProcesses; i++)
			auxiliary_buffs[i] = (unsigned char**)malloc(2 * sizeof(unsigned char*));
		for(int i = 0;i < nProcesses; i++)
			for(int j = 0;j < 2;j++)
				auxiliary_buffs[i][j] = (unsigned char*)malloc(dimensiune_line * sizeof(unsigned char*));

		unsigned char *aux = (unsigned char*)malloc(dimensiune_line * sizeof(unsigned char));

		// primim linii cheie si trimitem inter-process
		for(i = 0; i < no_filters - 1; i++){
			MPI_Status status;
			if (i <= no_filters - 2 && num_workers > 1){ 
				
				for(int j = 0; j < cnt; j++){
					MPI_Recv(aux, dimensiune_line, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
					int src = status.MPI_SOURCE;
					int tag = status.MPI_TAG;
					memcpy(auxiliary_buffs[src][tag], aux, dimensiune_line);
				}

				for(int j = 1;j < nProcesses;j++){
					if(j == 1)
						MPI_Ssend(auxiliary_buffs[j + 1][0], dimensiune_line, MPI_UNSIGNED_CHAR, j, 1, MPI_COMM_WORLD);

					else if(j > 1 && j < nProcesses - 1){
						MPI_Ssend(auxiliary_buffs[j - 1][1], dimensiune_line, MPI_UNSIGNED_CHAR, j, 0, MPI_COMM_WORLD);
						MPI_Ssend(auxiliary_buffs[j + 1][0], dimensiune_line, MPI_UNSIGNED_CHAR, j, 1, MPI_COMM_WORLD);
					}
					else if (j == nProcesses - 1)
						MPI_Ssend(auxiliary_buffs[j - 1][1], dimensiune_line, MPI_UNSIGNED_CHAR, j, 0, MPI_COMM_WORLD);

				}

				MPI_Barrier(MPI_COMM_WORLD);
			}
		
		}
		free(aux);
		
		// pentru ultimul filtru de aplicat

		FILE *out = fopen(argv[2],"wr");
		char header[100];
		sprintf(header,"P%c\n%d %d\n255\n",type,width,heigth);
		fwrite(header,1,strlen(header),out);


		// apply last filter
		MPI_Status status;

		unsigned char *block_aux = (unsigned char*)malloc(max_dimension * sizeof(unsigned char));

		for(int i = 1; i < nProcesses; i++){
			MPI_Recv(block_aux, max_dimension, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
			int src = status.MPI_SOURCE;
			
			int dim =  (end_recv_list[src]  - start_recv_list[src] + 1) * width;
			
			if(type == '6')
				dim = dim * 3;

			buffer_list[src] = (unsigned char*)malloc(dim * sizeof(unsigned char));
			dimensions[src] = dim;

			memcpy(buffer_list[src], block_aux, dim);
		}


		for(i = 1; i < nProcesses; i++){
			if(type == '5')
				fwrite(buffer_list[i],sizeof(unsigned char),dimensions[i],out);
			else
				fwrite(buffer_list[i],sizeof(unsigned char),dimensions[i],out);
		}

		//dezaloca auxiliary_buffs

		for(int i = 0;i < nProcesses; i++){
			for(int j = 0; j < 2;j++){
				free(auxiliary_buffs[i][j]);
			}
			free(auxiliary_buffs[i]);
		}
		free(auxiliary_buffs);
		

		for(int i = 1; i < nProcesses; i++)
			free(buffer_list[i]);
		free(buffer_list);
		
		free(block_aux);

		
	}
	else{
		omp_set_num_threads(num_threads);
		image input;
		image copy;
		int num_workers = nProcesses - 1;
			//primeste datele imaginii
		MPI_Bcast(&heigth,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&width,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&type,1,MPI_CHAR,0, MPI_COMM_WORLD);


		    // calculam ce ar trb sa primim
		int start = (rank == 1)?0: (rank-1) * (heigth/num_workers) - 1;
		int end = (rank == nProcesses - 1)?heigth - 1:rank * (heigth/num_workers);

			// inclusively
		int new_start_line = 1;
		int new_end_line = end - start - 1;


			//primeste bucata aferenta din imagine
		unsigned char *buffer;
		int dimension = (end - start + 1) * width;
		dimension = (type == '5')?dimension:3 * dimension;
		int dimensiune_line = dimension/(end - start + 1);
		buffer = (unsigned char*)malloc(dimension*sizeof(unsigned char));

		MPI_Recv(buffer,dimension,MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);


		//construieste imaginea
		input.heigth = end - start + 1;
		input.width = width;
		input.type = type;

		copy.heigth = end - start + 1;
		copy.width = width;
		copy.type = type;

		input.matrix = generate_image(input.heigth, input.width, input.type);
		copy.matrix = generate_image(input.heigth, input.width, input.type);
	
		// citeste imaginea
		if(type == '5'){
			black **aux = (black**)input.matrix;
			black **aux_copy = (black**)copy.matrix;
			for(i = 0;i < end - start + 1; i++)
				for(j = 0; j < input.width; j++){
					aux[i][j].white_black = buffer[i * width + j];
					aux_copy[i][j].white_black = buffer[i * width + j];
				}
		}
		else if(type == '6'){

			color **aux = (color**)input.matrix;
			color **aux_copy = (color**)copy.matrix;
			for(i = 0; i < end - start + 1; i++)
				for(j = 0; j < input.width; j++){
					aux[i][j].r = buffer[3 * (i*width+j)];
					aux[i][j].g = buffer[3 * (i*width+j) + 1];
					aux[i][j].b = buffer[3 * (i*width+j) + 2];

					aux_copy[i][j].r = buffer[3 * (i*width+j)];
					aux_copy[i][j].g = buffer[3 * (i*width+j) + 1];
					aux_copy[i][j].b = buffer[3 * (i*width+j) + 2];
				}

		}

		//aplica filtrele	
			for(i = 0;i < no_filters - 1; i++){
				applyFilter(&input, &copy, argv[i + 3], 1, input.heigth - 1);
				if (num_workers > 1){
					if(rank == 1){
						unsigned char *buff = preparePackage(&input, end - start - 1, end - start);
						MPI_Ssend(buff, dimensiune_line, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
						free(buff);

					}
					else if(rank < nProcesses - 1){
						unsigned char *buff = preparePackage(&input,1,2);
						MPI_Ssend(buff, dimensiune_line, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
						free(buff);
						buff = preparePackage(&input, end - start - 1, end - start);
						MPI_Ssend(buff, dimensiune_line, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
						free(buff);
					}

					else if(rank == nProcesses -1){
						unsigned char *buff= preparePackage(&input,1,2);
						MPI_Ssend(buff, dimensiune_line, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
						free(buff);
					}

					

					if(rank == 1){
						MPI_Recv(buffer, dimensiune_line, MPI_UNSIGNED_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						transfer_to_matrix(&input,buffer,end - start);
					}

					else if(rank < nProcesses - 1){
						MPI_Status status;
						for(int cnt = 0; cnt < 2; cnt++){
							MPI_Recv(buffer, dimensiune_line, MPI_UNSIGNED_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
							if(status.MPI_TAG == 0)
								transfer_to_matrix(&input,buffer,0);
							else
								transfer_to_matrix(&input,buffer,end - start);
						}
						
					}

					else if(rank == nProcesses -1){
						MPI_Recv(buffer, dimensiune_line, MPI_UNSIGNED_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
						transfer_to_matrix(&input,buffer,0);
			
					}

					MPI_Barrier(MPI_COMM_WORLD);
				}

			}

		applyFilter(&input, &copy, argv[no_filters + 2], 1, input.heigth - 1);
		//------------------------
		
		int l1 = (rank == 1)?0:1;
		int l2 = (rank == nProcesses - 1)?end - start + 1: end - start;
		dimension = (type == '5')?width*(l2 - l1):3*(l2 - l1)*width;

		//prepare buffer
		buffer = preparePackage(&input,l1,l2);
		MPI_Ssend(buffer, dimension, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
		free(buffer);
	
	}

	MPI_Finalize();

	return 0;
}
