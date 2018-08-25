#include "self-checksumming/DAGCheckersNetwork.h"
#include <time.h>
#include <algorithm>
#include <iomanip>

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
  std::vector<Function *> uniqueCheckees;
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
      if(std::find(uniqueCheckees.begin(), uniqueCheckees.end(), checkee)==uniqueCheckees.end()){
        j["allCheckees"].push_back(checkee->getName());
	uniqueCheckees.push_back(checkee);
      }
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

void topologicalSortUtil(int v, Function* F,
                         std::unique_ptr<bool[]> &visited,
                         std::list<Function*> &List,
			 const std::map<Function*,std::vector<Function*>> &checkerCheckeeMap,
			 std::vector<Function*> allFunctions) {
  // mark node as visited
  visited[v] = true;
  // recur for all vertices adjacent to this vertex
  auto it = checkerCheckeeMap.find(F);
  if (it == checkerCheckeeMap.end())
    return;

  for (auto i : it->second) {
    auto function_it = std::find(allFunctions.begin(), allFunctions.end(), i);
    int index = static_cast<int>(std::distance(allFunctions.begin(), function_it));
    if (!visited[index])
      topologicalSortUtil(index, *function_it, visited, List, checkerCheckeeMap, allFunctions);
  }
  List.push_back(allFunctions[v]);
}

std::vector<Function *> getAllFunctions(std::map<Function*,std::vector<Function*>> checkerCheckeeMap){

  std::vector<Function *> functions;
  for (auto &map : checkerCheckeeMap) {
    Function* checker = map.first;
    if(std::find(functions.begin(),functions.end(),checker)==functions.end())
      functions.push_back(checker);
    for (auto &checkee : map.second) {
      if(std::find(functions.begin(),functions.end(),checkee)==functions.end())
	functions.push_back(checkee);
    }
  }
  return functions;
}


std::list<Function*> DAGCheckersNetwork::getReverseTopologicalSort(std::map<Function*,std::vector<Function*>> checkerCheckeeMap) {
  std::list<Function *> List;
  // Mark all vetices as not visited
  std::vector<Function *> AllFunctions = getAllFunctions(checkerCheckeeMap);
  int V = static_cast<int>(AllFunctions.size());
  std::unique_ptr<bool[]> visited(new bool[V]);
  for (int i=0;i<V;i++)
    visited[i] = false;
  // call recursive helper to store the sort
  for (int i = 0; i < V; i++){
    auto function = AllFunctions[i];
    if (!visited[i])
      topologicalSortUtil(i,function ,visited, List,checkerCheckeeMap, AllFunctions);
  }
  dbgs() << "DAGCheckersNetwork::getReverseTopologicalSort freed visited\n";
  return List;
}

std::vector<Function *> randomComb(int connectivity,
                                   std::vector<Function *> allFunctions) {
  std::vector<unsigned int> indices(allFunctions.size());
  std::iota(indices.begin(), indices.end(), 0);

  auto rng = std::default_random_engine{};
  std::shuffle(indices.begin(), indices.end(), rng);
  std::vector<Function *> premutation;
  if (connectivity > allFunctions.size())
    connectivity = static_cast<int>(allFunctions.size());

  premutation.reserve(static_cast<unsigned long>(connectivity));
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
  
  std::vector<Function *> availableCheckees = sensitiveFunctions;  
  std::vector<Function *> availableCheckers = checkerFunctions;
  std::vector<Function *> visited;  
  for (auto &F : sensitiveFunctions) {
    dbgs()<<"Checker function:"<<F->getName()<<"\n";
    //availableCheckees.erase(
    //    std::remove(availableCheckees.begin(), availableCheckees.end(), F),
    //    availableCheckees.end());
    availableCheckers.erase(
        std::remove(availableCheckers.begin(), availableCheckers.end(), F),
        availableCheckers.end());
    //visited.push_back(F);

    if (availableCheckers.empty())
      break;
    int c = connectivity;
    if (std::find(sensitiveFunctions.begin(), sensitiveFunctions.end(), F) ==
        sensitiveFunctions.end()) {
      // when it's not a sensitive function, we randomly set the connectivity
      c = 0;//#48 nonsensitive functions only do checking do not get checked
      //rand() % (connectivity + 1);
      //dbgs() << "random connectivity for nonsensitive:" << c << "\n";
    }

    /*std::vector<Function *> possibleCheckees = sensitiveFunctions;
    for (auto &vis:visited){
      possibleCheckees.erase(
        std::remove(possibleCheckees.begin(), possibleCheckees.end(), vis),
        possibleCheckees.end());
    }*/

    checkeeChecker[F] = randomComb(c, availableCheckers);
    //if(checkeeChecker[F].size()!=c)
    errs()<<"C is set to "<<c<<" while size of checkees for "<<F->getName()<<" is "<<checkeeChecker[F].size()<<"\n";
    //exit(1);
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
