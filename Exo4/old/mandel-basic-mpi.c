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
//#define MAXX 10
#define MAXX 500
#define MAXY (MAXX * 3 / 4)

#define NX (2 * MAXX + 1)
#define NY (2 * MAXY + 1)

#define NBITER 550 /* Récursivité */
//#define NBITER 550
#define DATATAG 150

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

        if (rank == 0) {
                //printf("Master id %d on %d nodes starting ...\n", rank, size);

                int res;

                /* Begin User Program  - the master */
                //*
                int data[3], nb_pixel = (MAXX*2+1) * (MAXY*2+1);
                for(i = 0; i < nb_pixel; i++) {
                        MPI_Recv(&data, 3, MPI_INT, MPI_ANY_SOURCE, DATATAG, MPI_COMM_WORLD, &status);
                        //printf("Slave id %d has send : %d \n", status.MPI_SOURCE, data[2]);
                        //printf("%d: [%d,%d] -> [%d,%d] = %d\n", status.MPI_SOURCE, data[0], data[1], data[0] + MAXX, data[1] + MAXY, data[2]);
                        cases[data[0] + MAXX][data[1] + MAXY] = data[2];
                }
                //*/
                dump_ppm("mandel.ppm", cases);
                printf("Fini.\n");
        }

        else {
                //printf("Slave id %d on %d nodes starting ...\n", rank, size);
                /* On est l'un des fils */
                double x, y;
                int i, j, res, rc, data[3];
                //*
                for(i = -MAXX+(rank-1); i <= MAXX; i = i + nbslaves) { // Chacun fait une ligne
                        //printf("Slave id %d has start line : %d \n", rank,i);
                        for(j = -MAXY; j <= MAXY; j++) {
                                x = 2 * i / (double)MAXX;
                                y = 1.5 * j / (double)MAXY;

                                res = mandel(x, y);
                                data[0] = i; data[1] = j; data[2] = res;
//                                printf("[%d,%d] = %d\n", data[0], data[1], data[2]);
                                MPI_Send(&data, 3, MPI_INT, 0, DATATAG, MPI_COMM_WORLD);
//                                usleep(10000);
                        }
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
