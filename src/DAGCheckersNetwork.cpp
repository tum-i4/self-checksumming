#include "DAGCheckersNetwork.h"
#include <time.h>

using json = nlohmann::json;

void DAGCheckersNetwork::printVector(std::vector<int> vector) {
  for (int a : vector) {
    printf("%d,", a);
  }
  printf("\n");
}
void DAGCheckersNetwork::setLowerConnectivityAcceptance(bool value){
	this->accept_lower_connectivity = value;
}
std::map<Function *, std::vector<Function *>>
DAGCheckersNetwork::loadJson(std::string filePath, llvm::Module &module,
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
void DAGCheckersNetwork::dumpJson(
    const std::map<Function *, std::vector<Function *>> checkerToCheckee,
    std::string filePath, const std::list<Function *> reverseTopologicalSort) {
  // TODO: fix the problem with JSON dumper
  json j;
  j["allCheckees"] = json::array();
  for (auto checker : checkerToCheckee) {
    if (!checker.first) {
      dbgs() << "Null found\n";
      continue;
    }
    dbgs() << "dumpJson: dumping checker:" << checker.first->getName() << "\n";
    j["map"][checker.first->getName()] = json::array();
    for (auto checkee : checker.second) {
      j["map"][checker.first->getName()].push_back(checkee->getName());
      j["allCheckees"].push_back(checkee->getName());
    }
    dbgs() << "Dumped sucessfully\n";
  }
  j["topologicalsort"] = json::array();
  for (auto &tsort_checker : reverseTopologicalSort) {
    j["topologicalsort"].push_back(tsort_checker->getName());
  }
  std::cout << j.dump(4) << std::endl;
  std::ofstream o(filePath);
  o << std::setw(4) << j << std::endl;
}

/*void DAGCheckersNetwork::topologicalSortUtil(int v,
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
}*/

std::list<Function*> DAGCheckersNetwork::getReverseTopologicalSort(std::map<Function*,std::vector<Function*>> checkerCheckeeMap) {
  std::list<Function *> List;
  /*// Mark all vetices as not visited
  int V = this->AllFunctions;
  std::unique_ptr<bool[]> visited(new bool[V]);
  for (int i = 0; i < V; i++)
    visited[i] = false;
  // call recursive helper to store the sort
  for (int i = 0; i < V; i++)
    if (visited[i] == false)
      topologicalSortUtil(i, visited, List);
  dbgs() << "DAGCheckersNetwork::getReverseTopologicalSort freed visited\n";
  */return List;
}

std::vector<Function *> randomComb(int connectivity,
                                   std::vector<Function *> allFunctions) {
  std::vector<unsigned int> indices(allFunctions.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::random_shuffle(indices.begin(), indices.end());
  std::vector<Function *> premutation;
  if (connectivity > allFunctions.size())
    connectivity = allFunctions.size();
  for (int i = 0; i < connectivity; ++i) {
    premutation.push_back(allFunctions[indices[i]]);
  }
  return premutation;
}

std::map<Function *, std::vector<Function *>>
DAGCheckersNetwork::constructProtectionNetwork(
    std::vector<Function *> sensitiveFunctions,
    std::vector<Function *> checkerFunctions, int connectivity) {

  std::map<Function *, std::vector<Function *>> checkeeChecker;
  std::map<Function *, std::vector<Function *>> checkerCheckee;
  std::vector<Function *> availableCheckers = checkerFunctions;
  for (auto &F : sensitiveFunctions) {
    /*availableCheckers.erase(
        std::remove(availableCheckers.begin(), availableCheckers.end(), F),
        availableCheckers.end());
    if (availableCheckers.size() == 0)
      break;*/
    int c = connectivity;
    /*if (std::find(sensitiveFunctions.begin(), sensitiveFunctions.end(), F) ==
        sensitiveFunctions.end()) {
      // when it's not a sensitive function, we randomly set the connectivity
      c = 0;//#48 nonsensitive functions only do checking do not get checked
      //rand() % (connectivity + 1);
      dbgs() << "random connectivity for nonsensitive:" << c << "\n";
    }*/
    checkeeChecker[F] = randomComb(c, availableCheckers);
  }

  for (auto &map : checkeeChecker) {
    auto &checkee = map.first;
    dbgs() << "Checkee:" << checkee->getName() << "\n";
    if (std::find(sensitiveFunctions.begin(), sensitiveFunctions.end(), checkee) !=
        sensitiveFunctions.end()) {
      // this is a sensitive function assure that the number of checkers is
      // equal to the connectivity level
      if (map.second.size() != connectivity) {
	if(!accept_lower_connectivity){
          errs() << "DAGCheckersNetwork: connectivity level is not preserved for "
                    "sensitive functions\n";
          exit(1);
	} else {
          dbgs() << "DAGCheckersNetwork: connectivity level is not preserved for "<<checkee->getName()<<"\n";
         
	}
      }
    }
    for (auto &checker : map.second) {
      checkerCheckee[checker].push_back(checkee);
      dbgs() << checker->getName() << ",";
    }
    dbgs() << "\n";
  }
  return checkerCheckee;
}
