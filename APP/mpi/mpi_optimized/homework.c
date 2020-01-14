#include <time.h>
#include <unistd.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	free(buffer);
	fscanf(in, "%d\n%d\n%d\n", &img->width, &img->heigth, &img->maxval);

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

void applyFilter(char *filter,image *img,int l1,int l2){

	int i,j;
	int heigth = img->heigth;
	int width = img->width;
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

	float pondere = 0;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			pondere += v[i][j];
	if(strcmp(filter,"emboss") == 0)
		pondere = 1;


	if(img->type == '5'){
		
		black **aux = (black**)generate_image(l2-l1,width,img->type);
		black **copy = (black**)img->matrix;
		
		for(i=l1;i<l2;i++){
			for(j=1;j<width-1;j++){
				float sum = 0;
				for(int k=-1;k<=1;k++){
					for(int l=-1;l<=1;l++){
						sum += copy[i+k][j+l].white_black * (v[1+k][1+l]/pondere);

					
					}
				}
				aux[i-l1][j].white_black = (unsigned char)(sum);				
			}
		}

		for(i = l1; i < l2;i++){
			for(j = 1;j < width-1; j++){	
				black **helper = (black**)img->matrix;
				memcpy(&helper[i][j].white_black,&aux[i-l1][j].white_black,sizeof(unsigned char));
			}
		}
		
		for(i = 0; i < l2-l1; i++)
			free(aux[i]);
		free(aux);		

	}



	if(img->type == '6'){
		color **aux = (color**)generate_image(l2-l1,width,img->type);
		color **copy = (color**)img->matrix;
		
		for(i = l1; i < l2;i++){
			for(j = 1;j < width-1;j++){
				float sum_r = 0;
				float sum_g = 0;
				float sum_b = 0;

				for(int k = -1; k <= 1; k++){
					for(int l =-1 ;l <= 1; l++){
						sum_r += copy[i+k][j+l].r * (v[1+k][1+l]/pondere);
						sum_g += copy[i+k][j+l].g * (v[1+k][1+l]/pondere);
						sum_b += copy[i+k][j+l].b * (v[1+k][1+l]/pondere);
					}
				}
				aux[i-l1][j].r = (unsigned char)(sum_r);
				aux[i-l1][j].g = (unsigned char)(sum_g);
				aux[i-l1][j].b = (unsigned char)(sum_b);	
					
			}
		}

		for(i = l1; i < l2;i++){
			for(j = 1; j < width-1; j++){	
				color **helper = (color**)img->matrix;
				memcpy(&helper[i][j].r,&aux[i-l1][j].r,sizeof(unsigned char));
				memcpy(&helper[i][j].g,&aux[i-l1][j].g,sizeof(unsigned char));
				memcpy(&helper[i][j].b,&aux[i-l1][j].b,sizeof(unsigned char));
			}
		}
		
		for(i = 0;i < l2-l1; i++)
			free(aux[i]);
		free(aux);
			
	}

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
				buffer[(i-l1)*width+j] = aux[i][j].white_black;
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



int main(int argc, char * argv[]) {
	int i,j;
    int nProcesses;
    int heigth,width;
    char type;
    int no_filters = argc-3;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
 
	image input;

	if(rank == 0){

		//citeste imaginea
		readInput(argv[1], &input);
		//-----------------------

		//info
		heigth = input.heigth;
		width = input.width;
		type = input.type;

		//-----------------------

		if(nProcesses != 1){
			MPI_Bcast(&heigth,1,MPI_INT,0, MPI_COMM_WORLD);
			MPI_Bcast(&width,1,MPI_INT,0, MPI_COMM_WORLD);
			MPI_Bcast(&type,1,MPI_CHAR,0, MPI_COMM_WORLD);
		}



		//pregatim bufferele in care vom primi informatiile de la procese
		unsigned char **buffer_list = (unsigned char**)malloc((nProcesses)*sizeof(unsigned char*));
		int dimensions[nProcesses];


		//trimitem pentru fiecare process
		int l1_root,l2_root;
		int extended_l1_root,extended_l2_root;

		for(i=0;i<nProcesses;i++){
			
			//calculam cate linii trimitem pentru fiecare

			//intervalul pe care il vom scrie in fisierul de out
			int current_line = (i == 0)?0:i * (heigth/nProcesses);
			int last_line = (i == nProcesses-1)?heigth:(i+1) * (heigth/nProcesses);


			int l1 = (i == 0)?1:i * (heigth/nProcesses);
			int l2 = (i == nProcesses-1)?heigth-1:(i+1) * (heigth/nProcesses);

			//intervalul extins bazat pe numarul de filtre.
			int extended_l1 = (l1 - no_filters <= 0)?1:l1-no_filters;
			int extended_l2 = (l2 + no_filters >= heigth-1)?heigth-1:l2+no_filters;
			int k,l;
			//--------------------------------------------

			dimensions[i] = (last_line-current_line)*width;
			dimensions[i] = (type == '5')?dimensions[i]:3*dimensions[i];

			if(i == 0){
				l1_root = current_line;
				l2_root = last_line;
				extended_l1_root = extended_l1;
				extended_l2_root = extended_l2;
			}

			else{
				//pregatim bufferul cu informatii
				
				unsigned char *buffer;
				int dimension = (extended_l2 - extended_l1+2) *width;
				dimension = (type == '5')?dimension:3*dimension;
				buffer = (unsigned char*)malloc(dimension*sizeof(unsigned char));

				if(type == '5'){
					black **aux = (black**)input.matrix;
					for(k=extended_l1-1;k<extended_l2+1;k++)
						for(l=0;l<width;l++)
							buffer[(k - extended_l1+1)*width+l] = aux[k][l].white_black;
				}
				else if(type == '6'){
					color **aux = (color**)input.matrix;
					for(k=extended_l1-1;k<extended_l2+1;k++)
						for(l=0;l<width;l++){
							buffer[3*((k-extended_l1+1)*width+l)] = aux[k][l].r ;
							buffer[3*((k-extended_l1+1)*width+l)+1] = aux[k][l].g ;
							buffer[3*((k-extended_l1+1)*width+l)+2] = aux[k][l].b ;
						}

				}
				//trimite catre fiecare proces
				MPI_Ssend(buffer,dimension, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
				free(buffer);
			}
			
		}



		for(i = 0;i < argc-3; i++)
			applyFilter(argv[i+3],&input,extended_l1_root,extended_l2_root);
		buffer_list[0] = preparePackage(&input,l1_root,l2_root);

			

		//calculam dimensiunea maxima posibila
		int max_dimension = -1;
		for(i = 0;i < nProcesses; i++)
			if(dimensions[i] > max_dimension)
				max_dimension = dimensions[i];

		int nr = 1;

		
		unsigned char *aux = (unsigned char*)malloc(max_dimension*sizeof(unsigned char));
		while(nr != nProcesses){
			nr++;
			MPI_Status status;
			MPI_Recv(aux, max_dimension*sizeof(unsigned char),MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,&status);
			i = status.MPI_SOURCE;
			buffer_list[i] = (unsigned char*)malloc(dimensions[i]*sizeof(unsigned char));
			memcpy(buffer_list[i],aux,dimensions[i]);
		}
		free(aux);

	
		
		//scriere in fisier


		
		FILE *out = fopen(argv[2],"wr");
		char header[100];
		sprintf(header,"P%c\n%d %d\n255\n",type,width,heigth);
		fwrite(header,1,strlen(header),out);


		for(i = 0; i < nProcesses; i++){

			if(type == '5'){

				fwrite(buffer_list[i],sizeof(unsigned char),dimensions[i],out);
			}
			else{

				fwrite(buffer_list[i],sizeof(unsigned char),dimensions[i],out);
			}
		}

		

		for(i = 0;i < nProcesses; i++)
			free(buffer_list[i]);
		free(buffer_list);

		fclose(out);
	}
	else{
			//primeste datele imaginii
		MPI_Bcast(&heigth,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&width,1,MPI_INT,0, MPI_COMM_WORLD);
		MPI_Bcast(&type,1,MPI_CHAR,0, MPI_COMM_WORLD);

		    // calculam ce ar trb sa primim
		int current_line = (rank == 0)?0:rank * (heigth/nProcesses);
		int last_line = (rank == nProcesses-1)?heigth:(rank+1) * (heigth/nProcesses);


		int l1 = (rank == 0)?1:rank * (heigth/nProcesses);
		int l2 = (rank == nProcesses-1)?heigth-1:(rank+1) * (heigth/nProcesses);


		int extended_l1 = (l1 - no_filters <= 1)?1:l1-no_filters;
		int extended_l2 = (l2 + no_filters >= heigth-1)?heigth-1:l2+no_filters;

			//primeste bucata aferenta din imagine
		unsigned char *buffer;
		int dimension = (extended_l2 - extended_l1+2) *width;
		dimension = (type == '5')?dimension:3*dimension;
		buffer = (unsigned char*)malloc(dimension*sizeof(unsigned char));

		MPI_Recv(buffer,dimension,MPI_UNSIGNED_CHAR,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);



		//construieste imaginea
		input.heigth = (extended_l2 - extended_l1+2);
		input.width = width;
		input.type = type;
		input.matrix = generate_image(input.heigth,input.width,input.type);
		if(type == '5'){
			black **aux = (black**)input.matrix;
			for(i=0;i<input.heigth;i++)
				for(j=0;j<input.width;j++)
					aux[i][j].white_black = buffer[i*width+j];
		}
		else if(type == '6'){
			color **aux = (color**)input.matrix;
			for(i=0;i<input.heigth;i++)
				for(j=0;j<input.width;j++){
					aux[i][j].r = buffer[3*(i*width+j)];
					aux[i][j].g = buffer[3*(i*width+j)+1];
					aux[i][j].b = buffer[3*(i*width+j)+2];
				}

		}
		free(buffer);

		

		//aplica filtrele	
		for(i = 0;i < argc-3; i++){
			applyFilter(argv[i+3],&input,1,extended_l2-extended_l1+1);
		}
		
		//------------------------

		//dimensiunea
		dimension = (type == '5')?width*(last_line-current_line):3*(last_line-current_line)*width;

		//prepare buffer

		buffer = preparePackage(&input,current_line - extended_l1+1,last_line-extended_l1+1);
		//-------------------------


		MPI_Ssend(buffer,dimension, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
		free(buffer);
	
	}

	MPI_Finalize();

	return 0;
}
