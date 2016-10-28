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
#define BLOCKSIZE NY * 1/3 //4/3 //1 Line //TODO

static int mandel(double, double);

int dump_ppm(const char *, int[NX][NY]);
int *next(int[2]);
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
        int hasnext = NX*NY;

        if (rank == 0) {
                //printf("Master id %d on %d nodes starting ...\n", rank, size);

                /* Begin User Program  - the master */
                int result[BLOCKSIZE], counter[nbslaves], lastblock[2];

                lastblock[0] = -1; //Line
                lastblock[1] = -1; //Start

                for(int a=0; a < nbslaves; a++ ) {
                        //int block[2] = next(lastblock);
                        if(hasnext <= 0) {
                                continue; // On a moins de ligne que de worker
                        }
                        // printf("Send line %d to slave number %d for blocksize %d \n", line, a+1, BLOCKSIZE);
                        MPI_Send(next(lastblock), 2, MPI_INT, a+1, INSTRUCTIONTAG, MPI_COMM_WORLD);
                        hasnext -= BLOCKSIZE;
                        //sleep(1);
                        counter[a] = 0; //Init counter
                        busyslaves++;
                        printf("Status  hasnext : %d %d \n",hasnext, BLOCKSIZE);
                }
                while (busyslaves>0) {
                        //printf("Attente  maitre (busyslave :%d) ...\n",busyslaves);
                        int data[2];
                        MPI_Recv(&data, 2, MPI_INT, MPI_ANY_SOURCE, INSTRUCTIONTAG, MPI_COMM_WORLD, &status); //ID Sender
                        printf("Getting data from : %d for line %d and size of %d from %d\n", status.MPI_SOURCE, data[0], BLOCKSIZE, data[1]);
                        MPI_Recv(&result, BLOCKSIZE, MPI_INT, status.MPI_SOURCE, DATATAG, MPI_COMM_WORLD, &status); // Data
                        //*
                        counter[status.MPI_SOURCE-1]++;
                        int n = 0; //Line up
                        printf("Start writing line %d/%d ...\n",(data[0]),NX);
                        for( i=data[1]; i < data[1]+BLOCKSIZE; i++ ) {
                                if(i>=(NY+n*NY)) {
                                        n++; //Next line
                                        printf("Writing to next line %d ...\n",(data[0]+n));
                                }
                                if((data[0]+n) >= NX) { //TODO
                                        break;
                                }
                                cases[data[0]+n][i%NY] = result[i]; //TODO detect excess at end
                                //printf ("x:%d y:%d, id:%d = %d \n",data[0]+n,i%NY,i,result[i]);
                        }
                        //*/
                        //strncpy(cases[data[0]], result, BLOCKSIZE);
                        busyslaves--;
                        //if((line+1) <NX) {  //TODO handle end
                        printf("Status  hasnext : %d %d \n",hasnext, BLOCKSIZE);
                        if(hasnext > 0) {
                                //int block[2] = next(lastblock);
                                //printf("Send line %d/%d start %d/%d to slave number %d for blocksize %d \n", block[0], NX, block[1], NY, status.MPI_SOURCE, BLOCKSIZE);
                                MPI_Send(next(lastblock), 2, MPI_INT, status.MPI_SOURCE, INSTRUCTIONTAG, MPI_COMM_WORLD);
                                hasnext -= BLOCKSIZE;
                                //sleep(1);
                                busyslaves++;
                        }
                }
                for( i=1; i < nbslaves+1; i++ ) {
                        int data[2];
                        data[0] = -1; //Close signal
                        MPI_Send(data, 2, MPI_INT, i, INSTRUCTIONTAG, MPI_COMM_WORLD);
                }
                dump_ppm("mandel.ppm", cases);
                printf("Fini.\n");
                int total = 0;
                for(int a=0; a < nbslaves; a++ ) {
                        total += counter[a];
                        printf("Slave %d have done %d blocks.\n",a+1,counter[a]);
                }
                printf("Total : %d blocks (%d).\n",total, BLOCKSIZE);
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
                        printf("Start generation of line %d on slave number %d for blocksize %d  at start %d \n", data[0], rank, BLOCKSIZE, data[1]);
                        int tmp0 = data[0]; //TODO why data seems to be corrupted at some time?
                        int tmp1 = data[1]; //TODO why ?

                        i = data[0]-MAXX; // Line number
                        int n = 0; //Line up

                        for(j = data[1] /* Start */; j < data[1]+BLOCKSIZE /* Start + blocksize */; j++) {
                                if(n<0) {
                                        printf("Test n %d\n", n);
                                        n=0;
                                } else if (j>=(NY+n*NY)) {
                                        n++; //Next line
                                        printf("%d Slave id %d Generate next line (%d) ... %d %d %d %d\n",(j>=(NY+(n-1)*NY)), rank, NY, j, n, (NY+n*NY),(NY+(n-1)*NY));
                                };
                                if((i+n) > NX) { //TODO
                                        printf("Slave id %d next line overflow... %d %d %d\n", rank, j, n, (NY+n*NY));
                                        result[j] = 0;
                                        continue;
                                } else {
                                        x = 2 * (i+n) / (double)MAXX;
                                        y = 1.5 * ((j%NY)-MAXY) / (double)MAXY; //TODO detect excess at end
                                        result[j] = mandel(x, y);
                                }
                        }
                        data[0] = tmp0;
                        data[1] = tmp1;

                        printf("Sending result of line %d on slave number %d for blocksize %d  at start %d \n", data[0], rank, BLOCKSIZE, data[1]);
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

int *next(int lastblock[2]) //Up and deference nextblock
{
        int *nextblock = malloc(2 * sizeof(int));
        if (lastblock[0] == -1 && lastblock[1] == -1 ) {
                nextblock[0] = 0;
                nextblock[1] = 0;
        } else {
                nextblock[0] = lastblock[0]+((lastblock[1]+BLOCKSIZE)/NY);
                nextblock[1] = (lastblock[1]+BLOCKSIZE)%NY;
        }
        lastblock[0] = nextblock[0];
        lastblock[1] = nextblock[1];
        printf("Next block will be %d %d\n", nextblock[0], nextblock[1]);
        return nextblock;
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
