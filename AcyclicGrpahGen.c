#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_PER_RANK 1 /* Nodes/Rank: How 'fat' the DAG should be.  */
#define MAX_PER_RANK 4
#define MIN_RANKS 3    /* Ranks: How 'tall' the DAG should be.  */
#define MAX_RANKS 5
#define MIN(a,b) (((a)<(b))?(a):(b))
int main (void)
{
	int i, j, k,nodes = 0;
	srand (time (NULL));

	int ranks = MIN_RANKS
		+ (rand () % (MAX_RANKS - MIN_RANKS + 1));

	printf ("digraph {\n");
	int totalNodes = 4;  
	int desiredConnectivity= 2;
	for (i = 0; totalNodes>0; i++)
	{
		printf("before new nodes\n");
		/* New nodes of 'higher' rank than all nodes generated till now.  */
		int new_nodes = (rand () %  totalNodes+1);
		printf("nodes:%d new nodes:%d\n",nodes,new_nodes);
		/* Edges from old nodes ('nodes') to new ones ('new_nodes').  */
		for (j = 0; j < nodes; j++)
			for (k = 0; k < new_nodes; k++){
				int con=0;
				printf("%d %d\n",j,k);
				while(con++!=desiredConnectivity){
					int e = (rand () % new_nodes+1);
					printf ("  %d -> %d;\n", j, e); /* An Edge.  */
				}
			}
		nodes += new_nodes; /* Accumulate into old node set.  */
		totalNodes-=nodes; 
	}
	printf ("}\n");
	return 0;
}
