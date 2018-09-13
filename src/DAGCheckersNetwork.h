#include "CheckersNetworkBase.h"
#include "json.hpp"
#include "list"
#include "map"
#include "vector"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdlib.h>

#define MIN_PER_RANK 1 /* Nodes/Rank: How 'fat' the DAG should be.  */
#define MAX_PER_RANK 5
#define MIN_RANKS 3 /* Ranks: How 'tall' the DAG should be.  */
#define MAX_RANKS 5
#define PERCENT 30 /* Chance of having an Edge.  */

using namespace llvm;
class DAGCheckersNetwork : protected CheckersNetworkBase {
protected:
  //  std::map<int, std::vector<int>> checkerCheckeeMap;
  //void topologicalSortUtil(int v, std::unique_ptr<bool[]> &visited,
  //                         std::list<Function *> &List);
  //std::list<Function *> getReverseTopologicalSort(const std::map<Function *, std::vector<Function *>>);
  void printVector(std::vector<int> vector);
  // int AllFunctions;
  bool accept_lower_connectivity = false;
public:
  std::map<Function *, std::vector<Function *>> constructProtectionNetwork(std::vector<Function *> sensitiveFunctions,std::vector<Function *> checkerFunctions, int connectivity);
  void dumpJson(const std::map<Function *, std::vector<Function *>>,
                std::string filePath,
                const std::list<Function *> reverseTopologicalSort);
  std::list<Function*> getReverseTopologicalSort( std::map<Function*,std::vector<Function*>>);
  std::map<Function *, std::vector<Function *>>
  loadJson(std::string filePath, llvm::Module &module,
           std::list<Function *> &reverseTopologicalSort);
  void setLowerConnectivityAcceptance(bool);
};
