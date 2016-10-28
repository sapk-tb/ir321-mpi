/* Protocol */
//#define ASKFORWORK 1000
#define WORK 2000
#define RESULT 3000
#define FINISH -1
#define UNSET -2

/* Protocol Format */
#define BLOCKSIZE 10
typedef struct Task {
        int start;
        int size;
} Task;
typedef struct Result {
        int start;
        int size;
        int data[BLOCKSIZE];
} Result;

/* Mandel generation */
#define MAXX 50 /* Padding */ /* N'hesitez pas a changer MAXX .*/
#define MAXY (MAXX * 3 / 4)
#define NBITER 550 /* Récursivité */

/* Image specs */
#define NX (2 * MAXX + 1)
#define NY (2 * MAXY + 1)
