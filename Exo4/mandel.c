#include <stdio.h>

#define MIN(_x, _y) ((_x) > (_y) ? (_y) : (_x))
//static int mandel(double, double);
//int dump_ppm(const char *, int[NX][NY]);


/* Function to compute a point - the number of iterations plays a central role here */
int mandel(double cx, double cy) {
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

/* The image computer can be transformed in a ppm file so to be seen with xv */
int dump_ppm(const char *filename, int valeurs[NX][NY]) {
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
