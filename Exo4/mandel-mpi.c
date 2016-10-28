#include <stdio.h>
#include "const.h"
#include "mpi.h"
#include "mandel.c"

#define DEBUG 1
#define LOG(fmt, ...)  DEBUG && printf(fmt, __VA_ARGS__)
//#if DEBUG
//debug-mode only snippet go here.
//#endif

int main(int argc, char *argv[]) {

        int rank, size;
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        if (rank == 0) {
                return master(rank, size -1);
        } else {
                return slave(rank);
        }
}

Task *nextTask(Task *last, int total){
        Task *next = malloc(sizeof(Task));
        //LOG("nextTask debug : lastTask  start:%d, size:%d \n", last->start, last->size);
        if( last->start + last->size== total) {
                next->start = FINISH;
                return next;
        }

        if (last->start == UNSET) {
                next->start = 0;
        }else{
                next->start = last->start + last->size;
        }

        next->size = BLOCKSIZE;
        if ((next->start + next->size) > total) { //Detect overflow
                next->size = total - next->start;
        }
        //LOG("nextTask debug : -> nextTask  start:%d, size:%d \n", next->start,next->size);
        return next;
}
int master(int id, int NbSlave){
        LOG("Master %d : Starting ...\n", id);
        int pixelmap[NX][NY];
        int pixels = NX*NY;
        LOG("Master %d : %d pixels to generate ...\n", id, pixels);

        Task *lastTask = malloc(sizeof(Task)); lastTask->start = UNSET;

        int runningSlave = 0;
        for (int idSlave = 1; idSlave <= NbSlave; idSlave++) {
                /* Init slave action */
                lastTask = nextTask(lastTask, pixels);
                if(lastTask->start == FINISH) {
                        break; //We give all block allready
                }
                MPI_Send(lastTask, 2, MPI_INT, idSlave, WORK, MPI_COMM_WORLD);
                runningSlave++;
        }
        LOG("Master %d : Init of %d slave(s) done !\n", id, runningSlave);

        while(runningSlave>0) {
                MPI_Status status;
                Task task;
                Result result;
                LOG("Master %d : Waiting for result from %d runnning slave(s)\n",id, runningSlave);

                MPI_Recv(&result, sizeof(Result)-1, MPI_INT, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &status); // Data
                LOG("Master %d : Data receive from : %d for task (%d,%d)\n",id,status.MPI_SOURCE, result.start, result.size);
                runningSlave--;
                //sleep(2);
                int end = result.start+result.size;
                for (int i = result.start; i < end; i++) {
                  pixelmap[i/NY][i%NY] = result.data[i-result.start]; //TODO check ordering
                  LOG("Master %d : Storing pixel (%d,%d)\n",id,i/NY,i%NY);

                }

                lastTask = nextTask(lastTask, pixels);
                if(lastTask->start != FINISH) { //We still have works to do
                        MPI_Send(lastTask, 2, MPI_INT, status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
                        runningSlave++;
                }
        }
        LOG("Master %d : Generation Finished\n", id);
        LOG("Master %d : %d pixels generated ...\n", id, pixels);

        //Task *endTask = malloc(sizeof(Task)); endTask->start = FINISH; endTask->size = 0; //Should not be needed but just in case (could is lastTask)
        for (int idSlave = 1; idSlave <= NbSlave; idSlave++) {
                /* Close slaves at this point lastTask.start == FINISH */
                lastTask->start = FINISH;
                LOG("Master %d : Send close message to slave %d (%d)\n", id, idSlave,lastTask->start);
                MPI_Send(lastTask, 2, MPI_INT, idSlave, WORK, MPI_COMM_WORLD);
        }
        LOG("Master %d : %d Slaves close message send\n", id, NbSlave);

        dump_ppm("mandel.ppm", pixelmap);
        LOG("Master %d : Data dumped\n", id);

        MPI_Finalize();
        LOG("Master %d : Finished\n", id);
        return 0;
}

int slave(int id){
        Task task;
        MPI_Status status;
        LOG("Slave %d : Starting ...\n", id);

        while (1) { //Execution loop
                LOG("Slave %d : Waiting for order ...\n", id);
                MPI_Recv(&task, 2, MPI_INT, 0, WORK, MPI_COMM_WORLD, &status); //Wait for work
                if(task.start == FINISH) {
                        LOG("Slave %d : Received order to end\n", id);
                        break; //Close execution loop
                }

                LOG("Slave %d : Received task (%d,%d)\n", id, task.start, task.size);

                int x,y;
                Result result;
                result.start = task.start; result.size = task.size; //Init

                for (int i = 0; i < task.size; i++) {
                        posToXY(i+task.start,&x,&y);// Set x and y accordingly to position in global buffer
                        result.data[i] = mandel(x, y); //Generate point and save in output buffer
                }
                LOG("Slave %d : Generation of task (%d,%d) finished\n", id, task.start, task.size);
                //MPI_Send(&task, 2, MPI_INT, 0, WORK, MPI_COMM_WORLD); //Send back instruction
                MPI_Send(&result, sizeof(Result)-1, MPI_INT, 0, RESULT, MPI_COMM_WORLD); //Send result
                LOG("Slave %d : Result of task (%d,%d) sended\n", id, task.start, task.size);
        }

        LOG("Slave %d : Finished\n", id);
        MPI_Finalize();
        return 0;
}

void posToXY(int position, int *X, int *Y){
        *X = 2 * (int)(position/NY) / (double)MAXX;
        *Y = 1.5 * ((position%NY)-MAXY) / (double)MAXY;
        //LOG("Debug XY : %d, %d \n",(int)(position/NY),((position%NY)-MAXY));
}
