#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <cstdlib>
#include <map>
#define MIN_PER_RANK 1 /* Nodes/Rank: How 'fat' the DAG should be.  */
#define MAX_PER_RANK 5
#define MIN_RANKS 3    /* Ranks: How 'tall' the DAG should be.  */
#define MAX_RANKS 5
#define PERCENT 30     /* Chance of having an Edge.  */


void printVector(std::vector<int> vector){
	for (int a : vector) {
		printf("%d,",a);
	}
	printf("\n");
}

std::map<int, std::vector<int>> constructAcyclicCheckers(int totalNodes, int desiredConnectivity){
	std::map<int, std::vector<int>> checkerCheckeeMap;
	int i, j, k,nodes = 0;
	srand (time (NULL));

	int ranks = MIN_RANKS
		+ (rand () % (MAX_RANKS - MIN_RANKS + 1));

	printf ("digraph {\n");
	for (i = 0; nodes<totalNodes; i++)
	{
		/* New nodes of 'higher' rank than all nodes generated till now.  */
		//int new_nodes = MIN_PER_RANK
		//	+ (rand () % (MAX_PER_RANK - MIN_PER_RANK + 1));
		int new_nodes = rand() % totalNodes/2 +1;
		int remainingNodes = totalNodes - nodes;
		new_nodes = new_nodes > remainingNodes? remainingNodes:new_nodes;
		printf("newNodes:%d\n", new_nodes);
		/* Edges from old nodes ('nodes') to new ones ('new_nodes').  */
		for (j = 0; j < nodes; j++){
			std::vector<int> availableNodes = {};
			auto checker = checkerCheckeeMap.find(j);
			int connectivity = 0;
			if(checker!=checkerCheckeeMap.end())
				connectivity = checker->second.size();
			for (k = 0; k < new_nodes; k++)
				if ( (rand () % 100) < PERCENT){
					printf ("  %d -> %d;\n", j, k + nodes); /* An Edge.  */
					++connectivity;
					std::vector<int> checkeeVector = {}; 
					auto checker = checkerCheckeeMap.find(j);
					if (checker!=checkerCheckeeMap.end()){
						checkeeVector = checker->second;
						checkeeVector.push_back(k+nodes);
					} else {
						checkeeVector.push_back(k+nodes);
					}
					checkerCheckeeMap[j] = checkeeVector;

					printVector(checkeeVector);
				}
				else {
					availableNodes.push_back(k); 
				}
			//Aim for getting desired connectivity
			for (int a : availableNodes){
				if(connectivity<desiredConnectivity){
					printf ("  %d -> %d;\n", j, a + nodes); /* An Edge.  */
					++connectivity;
					std::vector<int> checkeeVector = {};
					auto checker = checkerCheckeeMap.find(j);
					if (checker!=checkerCheckeeMap.end()){
						checkeeVector = checker->second;
						checkeeVector.push_back(a+nodes);
					} else {
						checkeeVector.push_back(a+nodes);
					}
					checkerCheckeeMap[j] = checkeeVector;

					printVector(checkeeVector);
	
				}
			}
		}		

		nodes += new_nodes; /* Accumulate into old node set.  */
		printf("nodes:%d total:%d\n",nodes, totalNodes);
	}

	return checkerCheckeeMap;
}

int main (int argc, char *argv[])
{
	int totalNodes = 0;
	int desiredConnectivity = 0;
	std::map<int, std::vector<int>> checkerCheckeeMap;;
	if( argc != 3 ) {
		printf("I need two cmd line arguments: totalNodes and desiredConnectivity\n");
		exit(1);
	} else {
		totalNodes = atoi(argv[1]);
		desiredConnectivity = atoi(argv[2]);
	}
	checkerCheckeeMap =constructAcyclicCheckers(totalNodes,desiredConnectivity); 
	return 0;
}
