#include <stdio.h>
#include "mpi.h"

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define ATTRIBUTE(x) __attribute__(x)
#else
#define ATTRIBUTE(x) /**/
#endif

#define MIN(_x, _y) ((_x) > (_y) ? (_y) : (_x))
#define ABS(_x) ((_x) >= 0 ? (_x) : -(_x))


/* N'hesitez pas a changer MAXX .*/ /* Résolution */
#define MAXX 500
//#define MAXX 500
#define MAXY (MAXX * 3 / 4)

#define NX (2 * MAXX + 1)
#define NY (2 * MAXY + 1)

#define NBITER 550 /* Récursivité */
//#define NBITER 550

#define DATATAG 150
#define INSTRUCTIONTAG 250
#define BLOCKSIZE NY //1 Line

static int mandel(double, double);

int dump_ppm(const char *, int[NX][NY]);
int cases[NX][NY];


int main(int argc, char *argv[])
{
        MPI_Status status;
        int i,j, num, rank, size, nbslaves;
        char inputstr [100],outstr [100];

        /* Start up MPI */

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        nbslaves = size -1;
        int busyslaves = 0;

        if (rank == 0) {
                //printf("Master id %d on %d nodes starting ...\n", rank, size);

                /* Begin User Program  - the master */
                int line = 0,data[2], result[BLOCKSIZE];

                for(int a=0; a < nbslaves; a++ ) {
                        if(NX < a) {
                                continue; // On a moins de ligne que de worker
                        }
                        // printf("Send line %d to slave number %d for blocksize %d \n", line, a+1, BLOCKSIZE);
                        data[0] = line; // Ligne
                        data[1] = 0; // Du début de la ligne
                        MPI_Send(&data, 2, MPI_INT, a+1, INSTRUCTIONTAG, MPI_COMM_WORLD);
                        //sleep(1);
                        busyslaves++;
                        line++;
                }
                while (busyslaves>0) {
                        // printf("Attente  maitre (busyslave :%d) ...\n",busyslaves);
                        MPI_Recv(&data, 2, MPI_INT, MPI_ANY_SOURCE, INSTRUCTIONTAG, MPI_COMM_WORLD, &status); //ID Sender
                        // printf("Getting data from : %d for line %d and size of %d from %d\n", status.MPI_SOURCE, data[0], BLOCKSIZE, data[1]);
                        MPI_Recv(&result, BLOCKSIZE, MPI_INT, status.MPI_SOURCE, DATATAG, MPI_COMM_WORLD, &status); // Data
                        //*
                        for( i=0; i < BLOCKSIZE; i++ ) {
                                cases[data[0]][i]= result[i];
                                //printf ("x:%d y:%d, id:%d = %d \n",data[0],i,i,result[i]);
                        }
                        //*/
                        //strncpy(cases[data[0]], result, BLOCKSIZE);
                        busyslaves--;
                        if((line+1) <NX) {
                                line++;
                                //printf("Send line %d to slave number %d for blocksize %d \n", line, status.MPI_SOURCE, BLOCKSIZE);
                                data[0] = line; // Ligne
                                data[1] = 0; // Du début de la ligne
                                MPI_Send(&data, 2, MPI_INT, status.MPI_SOURCE, INSTRUCTIONTAG, MPI_COMM_WORLD);
                                //sleep(1);
                                busyslaves++;
                        }
                }
                for( i=1; i < nbslaves+1; i++ ) {
                        data[0] = -1; //Close signal
                        MPI_Send(data, 2, MPI_INT, i, INSTRUCTIONTAG, MPI_COMM_WORLD);
                }
                dump_ppm("mandel.ppm", cases);
                printf("Fini.\n");
        }

        else {
                printf("Slave id %d on %d nodes starting ...\n", rank, size);
                /* On est l'un des fils */
                while (1) {
                        int data[2], i, j, result[BLOCKSIZE];
                        MPI_Recv(&data, 2, MPI_INT, 0, INSTRUCTIONTAG, MPI_COMM_WORLD, &status);
                        double x, y;
                        if(data[0] == -1) {
                                break; //Closing by master
                        }
                        //printf("Start generation of line %d on slave number %d for blocksize %d  at start %d \n", data[0], rank, BLOCKSIZE, data[1]);
                        i = data[0]-MAXX; // Line number
                        //printf("I : %d, Start %d, End %d of generation \n",i ,data[1]-MAXY, data[1]-MAXY+BLOCKSIZE);
                        for(j = data[1]-MAXY /* Start */; j < data[1]-MAXY+BLOCKSIZE /* Start + blocksize */; j++) {
                                x = 2 * i / (double)MAXX;
                                y = 1.5 * j / (double)MAXY;
                                //printf ("i:%d j:%d (x:%d,y:%d) = %d \n",i,j,x,y,result[j+MAXY]);
                                result[j+MAXY] = mandel(x, y);
                                //printf ("i:%d j:%d, id:%d = %d \n",i,j,j+MAXY,result[j+MAXY]);
                        }
                        MPI_Send(&data, 2, MPI_INT, 0, INSTRUCTIONTAG, MPI_COMM_WORLD); //Send back instruction
                        MPI_Send(&result, BLOCKSIZE, MPI_INT, 0, DATATAG, MPI_COMM_WORLD); //Then send data
                }
        }
        MPI_Finalize();
        return 0;
}



/* function to compute a point - the number of iterations
   plays a central role here */

int
mandel(double cx, double cy)
{
        int i;
        double zx, zy;
        zx = 0.0; zy = 0.0;
        for(i = 0; i < NBITER; i++) {
                double zxx = zx, zyy = zy;
                zx = zxx * zxx - zyy * zyy + cx;
                zy = 2 * zxx * zyy + cy;
                if(zx * zx + zy * zy > 4.0)
                        return i;
        }
        return -1;
}

/* the image commputer can be transformed in a ppm file so
   to be seen with xv */

int
dump_ppm(const char *filename, int valeurs[NX][NY])
{
        FILE *f;
        int i, j, rc;

        f = fopen(filename, "w");
        if(f == NULL) { perror("fopen"); exit(1); }
        fprintf(f, "P6\n");
        fprintf(f, "%d %d\n", NX, NY);
        fprintf(f, "%d\n", 255);
        for(j = NY - 1; j >= 0; j--) {
                for(i = 0; i < NX; i++) {
                        unsigned char pixel[3];
                        if(valeurs[i][j] < 0) {
                                pixel[0] = pixel[1] = pixel[2] = 0;
                        } else {
                                unsigned char val = MIN(valeurs[i][j] * 12, 255);
                                pixel[0] = val;
                                pixel[1] = 0;
                                pixel[2] = 255 - val;
                        }
                        rc = fwrite(pixel, 1, 3, f);
                        if(rc < 0) { perror("fwrite"); exit(1); }
                }
        }
        fclose(f);
        return 0;
}
