#include "self-checksumming/CheckersNetwork.h"
#include <time.h>

using json = nlohmann::json;

void CheckersNetwork::printVector(std::vector<int> vector) {
  for (int a : vector) {
    printf("%d,", a);
  }
  printf("\n");
}

std::map<Function *, std::vector<Function *>>
CheckersNetwork::loadJson(std::string filePath, llvm::Module &module,
                          std::list<Function *> &reverseTopologicalSort) {

  std::map<Function *, std::vector<Function *>> dump_map;
  // check if the file exists
  std::ifstream i(filePath);
  if (i.is_open()) {
    json j;
    i >> j;
    for (auto &checker : j["topologicalsort"]) {
      auto checkerFunc = module.getFunction(checker.get<std::string>());
      std::vector<Function *> func_checkee;
      std::cout << checker << "\n";
      auto &checkees = j["map"][checker.get<std::string>()];
      for (auto &checkee : checkees) {
        func_checkee.push_back(module.getFunction(checkee.get<std::string>()));
      }
      dump_map[checkerFunc] = func_checkee;

      reverseTopologicalSort.push_back(checkerFunc);
    }
    // json tsort = j["topologicalsort"];
    // for(auto& tsort_checker:tsort){
    // std::cout<<tsort_checker<<"\n";
    //}
  } else {
    dbgs() << "ERR. Could not open the provided CheckersNetwork file "
           << filePath << "\n";
  }
  return dump_map;
}
void CheckersNetwork::dumpJson(
    const std::map<Function *, std::vector<Function *>> checkerToCheckee,
    std::string filePath, const std::list<Function *> reverseTopologicalSort) {
  // TODO: fix the problem with JSON dumper
  json j;
  j["allCheckees"] = json::array();
  for (auto checker : checkerToCheckee) {
    j["map"][checker.first->getName()] = json::array();
    for (auto checkee : checker.second) {
      j["map"][checker.first->getName()].push_back(checkee->getName());
      j["allCheckees"].push_back(checkee->getName());
    }
  }
  j["topologicalsort"] = json::array();
  for (auto &tsort_checker : reverseTopologicalSort) {
    j["topologicalsort"].push_back(tsort_checker->getName());
  }
  std::cout << j.dump(4) << std::endl;
  std::ofstream o(filePath);
  o << std::setw(4) << j << std::endl;
}

void CheckersNetwork::topologicalSortUtil(int v,
                                          std::unique_ptr<bool[]> &visited,
                                          std::list<int> &List) {
  // mark node as visited
  visited[v] = true;
  // recur for all vertices adjacent to this vertex
  auto it = this->checkerCheckeeMap.find(v);
  if (it == this->checkerCheckeeMap.end())
    return;

  for (auto i = it->second.begin(); i != it->second.end(); ++i) {
    if (!visited[*i])
      topologicalSortUtil(*i, visited, List);
  }
  List.push_back(v);
}

std::list<int> CheckersNetwork::getReverseTopologicalSort() {
  std::list<int> List;
  // Mark all vetices as not visited
  int V = this->AllFunctions;
  std::unique_ptr<bool[]> visited(new bool[V]);
  for (int i = 0; i < V; i++)
    visited[i] = false;
  // call recursive helper to store the sort
  for (int i = 0; i < V; i++)
    if (visited[i] == false)
      topologicalSortUtil(i, visited, List);
  dbgs() << "CheckersNetwork::getReverseTopologicalSort freed visited\n";
  return List;
}

std::map<Function *, std::vector<Function *>>
CheckersNetwork::mapCheckersOnFunctions(
    const std::vector<Function *> allFunctions,
    std::list<Function *> &reverseTopologicalSort, llvm::Module &module) {

  this->AllFunctions = allFunctions.size();
  // TODO: This check does not make sense
  /*if (allFunctions.size() < this->checkerCheckeeMap.size()) {
    // total number of nodes cannot be greater than
    // all available functions
    printf("Error in number of nodes\n");
    exit(1);
  }*/

  std::map<Function *, std::vector<Function *>> dump_map;
  // TODO: Remove hardcoded checker checkee:
  /* auto *f1 = module.getFunction("f1");
   auto *f2 = module.getFunction("f2");
   auto *f3 = module.getFunction("f3");
   auto *f4 = module.getFunction("f4");

   std::vector<Function *> f1_checkee;
   f1_checkee.push_back(f3);
   f1_checkee.push_back(f4);

   std::vector<Function *> f2_checkee;
   f2_checkee.push_back(f3);
   f2_checkee.push_back(f4);

   dump_map[f1] = f1_checkee;
   dump_map[f2] = f2_checkee;
   reverseTopologicalSort.push_back(f1);
   reverseTopologicalSort.push_back(f2);
   return dump_map;*/

  // map functions on nodes
  std::map<int, Function *> internalMap;
  for (auto it = allFunctions.begin(); it != allFunctions.end(); ++it) {
    // mapping from nodes (numbers) to functions
    int node_index = std::distance(allFunctions.begin(), it);
    internalMap[node_index] = *it;
  }
  dbgs()
      << "CheckersNetwork:mapCheckersOnFunctions: internal mapping is done.\n";
  // dump function map
  for (auto checker : this->checkerCheckeeMap) {
    std::vector<Function *> checkee_map;
    for (int checkee_index : checker.second) {
      auto correspondingCheckeeFunc = internalMap[checkee_index];
      checkee_map.push_back(correspondingCheckeeFunc);
    }
    int checker_index = checker.first;
    auto correspondingCheckerFunc = internalMap[checker_index];
    dump_map[correspondingCheckerFunc] = checkee_map;
  }
  dbgs() << "CheckersNetwork:mapCheckersOnFunctions: functions are maped on "
            "the internal map.\n";
  // get reverse topological sort
  std::list<int> revTopSort = getReverseTopologicalSort();
  for (auto it = revTopSort.begin(); it != revTopSort.end(); ++it) {
    reverseTopologicalSort.push_back(internalMap[*it]);
  }
  dbgs() << "CheckersNetwork:mapCheckersOnFunctions: reverse topological sort "
            "is done.\n";
  return dump_map;
}
void CheckersNetwork::constructAcyclicCheckers(
    int totalNodes, int desiredConnectivity,
    std::vector<int> &actualConnectivity) {
  int i, j, k, nodes = 0;
  srand(time(NULL));

  int ranks = MIN_RANKS + (rand() % (MAX_RANKS - MIN_RANKS + 1));

  printf("digraph {\n");
  for (i = 0; nodes < totalNodes; i++) {
    /* New nodes of 'higher' rank than all nodes generated till now.  */
    // int new_nodes = MIN_PER_RANK
    //      + (rand () % (MAX_PER_RANK - MIN_PER_RANK + 1));
    int new_nodes = rand() % totalNodes / 2 + 1;
    int remainingNodes = totalNodes - nodes;
    new_nodes = new_nodes > remainingNodes ? remainingNodes : new_nodes;
    printf("newNodes:%d\n", new_nodes);
    /* Edges from old nodes ('nodes') to new ones ('new_nodes').  */
    for (j = 0; j < nodes; j++) {
      std::vector<int> availableNodes = {};
      auto checker = checkerCheckeeMap.find(j);
      int connectivity = 0;
      if (checker != checkerCheckeeMap.end())
        connectivity = checker->second.size();
      for (k = 0; k < new_nodes; k++)
        if ((rand() % 100) < PERCENT) {
          printf("  %d -> %d;\n", j, k + nodes); /* An Edge.  */
          ++connectivity;
          std::vector<int> checkeeVector = {};
          auto checker = checkerCheckeeMap.find(j);
          if (checker != checkerCheckeeMap.end()) {
            checkeeVector = checker->second;
            checkeeVector.push_back(k + nodes);
          } else {
            checkeeVector.push_back(k + nodes);
          }
          checkerCheckeeMap[j] = checkeeVector;

          printVector(checkeeVector);
        } else {
          availableNodes.push_back(k);
        }
      // Aim for getting desired connectivity
      for (int a : availableNodes) {
        if (connectivity < desiredConnectivity) {
          printf("  %d -> %d;\n", j, a + nodes); /* An Edge.  */
          ++connectivity;
          std::vector<int> checkeeVector = {};
          auto checker = checkerCheckeeMap.find(j);
          if (checker != checkerCheckeeMap.end()) {
            checkeeVector = checker->second;
            checkeeVector.push_back(a + nodes);
          } else {
            checkeeVector.push_back(a + nodes);
          }
          checkerCheckeeMap[j] = checkeeVector;

          printVector(checkeeVector);
        }
      }
    }

    nodes += new_nodes; /* Accumulate into old node set.  */
    printf("nodes:%d total:%d\n", nodes, totalNodes);
  }
  // Calculate the actual network connectivity
  for (auto const &entry : checkerCheckeeMap) {
    auto const &checkeesVector = entry.second;
    actualConnectivity.push_back(checkeesVector.size());
  }
}
