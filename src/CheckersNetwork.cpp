#include "CheckersNetwork.h"
#include <time.h>
void printVector(std::vector<int> vector){
	for (int a : vector) {
		printf("%d,",a);
	}
	printf("\n");
}

void topologicalSortUtil(int v, bool visited[], std::list<int> &List,
		const std::map<int, std::vector<int>> checkersNetwork){
	//mark node as visited
	visited[v] = true;
	//recur for all vertices adjacent to this vertex
	auto it = checkersNetwork.find(v);
	if (it== checkersNetwork.end()) return;

	for(auto i=it->second.begin();i!=it->second.end();++i){
		if(!visited[*i])
			topologicalSortUtil(*i,visited,List,checkersNetwork);
	} 
	List.push_back(v);
}

std::list<int> getReverseTopologicalSort(const std::map<int, std::vector<int>> checkersNetwork){
	std::list<int> List;
	//Mark all vetices as not visited
	int V = checkersNetwork.size();
	bool *visited = new bool[V];
	for(int i=0;i<V;i++)
		visited[i] = false;
	//call recursive helper to store the sort
	for(int i=0;i<V;i++)
		if (visited[i]==false)
			topologicalSortUtil(i,visited,List,checkersNetwork);


	return List;


}				

std::map<Function*, std::vector<Function*>> 
mapCheckersOnFunctions(const std::map<int, std::vector<int>> checkersNetwork,
		const std::vector<Function*> allFunctions, std::list<Function *> &reverseTopologicalSort){
	if(allFunctions.size()< checkersNetwork.size()){
		//total number of nodes cannot be greater than 
		//all available functions
		printf("Error in number of nodes\n");
		exit(1);
	}
	//map functions on nodes
	std::map<int, Function*> internalMap;
	for (auto it = allFunctions.begin(); it != allFunctions.end(); ++it) {
		//mapping from nodes (numbers) to functions
		int node_index = std::distance(allFunctions.begin(), it);
		internalMap[node_index]=*it;
	}
	//dump function map
	std::map<Function*, std::vector<Function*>> dump_map;
	for(auto checker: checkersNetwork){
		std::vector<Function*> checkee_map;
		for(int checkee_index:checker.second){
			auto correspondingCheckeeFunc = internalMap[checkee_index];
			checkee_map.push_back(correspondingCheckeeFunc);
		}
		int checker_index = checker.first;
		auto correspondingCheckerFunc = internalMap[checker_index];
		dump_map[correspondingCheckerFunc]=checkee_map; 
	}

	//get reverse topological sort
	std::list<int> revTopSort = getReverseTopologicalSort(checkersNetwork);
	for (auto it= revTopSort.begin();it!=revTopSort.end();++it){
		reverseTopologicalSort.push_back(internalMap[*it]);
	}
	return dump_map;

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
		//      + (rand () % (MAX_PER_RANK - MIN_PER_RANK + 1));
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

