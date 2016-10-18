#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include <dirent.h>

/*
void send(int rank, int tag, char *cmd){
	printf("I've sent  %s to  slave number %d w/ length %d \n", cmd, rank, strlen(cmd));
	printf("Process sending %s to %d\n", cmd, rank);
	MPI_Send(cmd, strlen(cmd)+1, MPI_CHAR, rank, tag, MPI_COMM_WORLD); 
}
*/
/*
void nextCMD(struct dirent* d){
	
}
*/
int main(int argc, char *argv[])
{
  MPI_Status status;
  int i,j, num, rank, size, tag, next, from, nbslaves;
  FILE *fp;
  DIR *dirp;
  int notover;
  int  receivedsize;

  int partialsize, totalsize, sender;
  char instr [100], base[256], cmd[256], buf[256], f[256][256];
  struct dirent *direntp;

  /* Start up MPI */

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
 
  /* Arbitrarily choose 201 to be our tag.  Calculate the */
  /* rank of the next process in the ring.  Use the modulus */
  /* operator so that the last process "wraps around" to rank */
  /* zero. */

  notover = 1;
  totalsize = 0;
  tag = 201;
  next = (rank + 1) % size;
  from = (rank + size - 1) % size;

  nbslaves = size - 1;

  int wait_for_nb_slaves=0;

  if (rank == 0) {

    /* Begin User Program - the master */

    dirp = opendir(argv[1]);
    if (dirp == NULL) {
      perror("Erreur sur ouverture repertoire");
      exit(1);
    }
    sprintf(base, "du -s %s/", argv[1]);

    //Generate list folder
/*
    i=0;
    while ( (direntp = readdir( dirp )) != NULL ) {
        i++;
	if(!strcmp(direntp->d_name, "."))
	    { i= i-1; continue;}
	if(!strcmp(direntp->d_name, ".."))
	    {i = i-1; continue;}
	strcpy(cmd, base);
	strcat(cmd, direntp->d_name);
	strcpy(f[i-1],cmd);
    }
//*/
//*
    //Init worker
    i=0;//i correspond to working slave
    for(int a=0 ; a < nbslaves ; a++ ) {
	if( (direntp = readdir( dirp )) != NULL ) {
		if(!strcmp(direntp->d_name, "."))
		    { a= a-1; continue;}
		if(!strcmp(direntp->d_name, ".."))
		    { a= a-1; continue;}
		strcpy(cmd, base);
		strcat(cmd, direntp->d_name);
		i++;
		printf("I've sent  %s to  slave number %d w/ length %d \n", cmd, i, strlen(cmd));
		printf("Process sending %s to %d\n", cmd, i);
		MPI_Send(cmd, strlen(cmd)+1, MPI_CHAR, i, tag, MPI_COMM_WORLD); 
	}
    }
//*/
//*
    while (i>0) {
	printf("Attente  maitre (%d) ...\n",i);
	MPI_Recv(&sender, 1, MPI_INT, MPI_ANY_SOURCE, tag+2, MPI_COMM_WORLD, &status);//ID Sender
	MPI_Recv(&partialsize, 1, MPI_INT, sender, tag+1, MPI_COMM_WORLD, &status); //Value
	totalsize = totalsize + partialsize;
	printf("I got %d from node %d \n",partialsize, sender);
	printf("Total %d  \n",totalsize);
        i--;

	if( (direntp = readdir( dirp )) != NULL ) { //We have folde
		if(!strcmp(direntp->d_name, "."))
		    { i= i-1; continue;}
		if(!strcmp(direntp->d_name, ".."))
		    {i = i-1; continue;}
		strcpy(cmd, base);
		strcat(cmd, direntp->d_name);
		printf("I've sent  %s to  slave number %d w/ length %d \n", cmd, sender, strlen(cmd));
		printf("Process sending %s to %d\n", cmd, sender);
		MPI_Send(cmd, strlen(cmd)+1, MPI_CHAR, sender, tag, MPI_COMM_WORLD); 
		i++;
	}
/*
		printf("I've sent  %s to  slave number %d w/ length %d \n", f[i], sender, strlen(f[i]));
		printf("Process sending %s to %d\n", f[i], sender);
		MPI_Send(f[i], strlen(f[i])+1, MPI_CHAR, sender, tag, MPI_COMM_WORLD); 
	}
*/
    }
 //*/ 
    printf("calcul termine pour le maitre\n");
    (void)closedir( dirp );
    printf("close dir  pour le maitre\n");
	
    for( i=1 ; i < nbslaves+1 ; i++ ) {
      MPI_Send(cmd, 0, MPI_CHAR, i, tag, MPI_COMM_WORLD); }

  } else {

    /* slaves */

    while (notover) {

      size = 0;
      /* Receive data from master */
      
      MPI_Recv(instr, 100, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);
      
      /* we call MPI_Get8count to determine the size of the message received */
      
      MPI_Get_count (&status, MPI_CHAR, &receivedsize);
      if ( receivedsize== 0) {
	notover = 0;
      }
      else {
	fp = popen(instr, "r");
	while (fgets(buf, BUFSIZ, fp) != NULL) {
	  size = size + atoi(buf);
	}
	fclose(fp);
	printf("slave %i: executed %s returning: %i\n", rank, instr, size);
	/* Send result to master */
	MPI_Send(&rank, 1, MPI_INT, 0, tag+2, MPI_COMM_WORLD);
	MPI_Send(&size, 1, MPI_INT, 0, tag+1, MPI_COMM_WORLD); 
      }
    }
    
  }
  printf("calcul finalise\n");
  
  MPI_Finalize();
  return 0;
}
