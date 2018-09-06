#include "list"
#include "map"
#include "vector"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "nlohmann/json.hpp"
#include <stdlib.h>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
class CheckersNetworkBase {
protected:
  //std::map<int, std::vector<int>> checkerCheckeeMap;
  //void topologicalSortUtil(int v, std::unique_ptr<bool[]> &visited, std::list<int> &List);
  //std::list<int> getReverseTopologicalSort();
  virtual void printVector(std::vector<int> vector) = 0;
  //int AllFunctions;

public:
  virtual std::map<Function*,std::vector<Function*>> constructProtectionNetwork(std::vector<Function*> sensitiveFunctions,std::vector<Function*> allFunctions,int connectivity) = 0;
  virtual std::list<Function*> getReverseTopologicalSort( std::map<Function*,std::vector<Function*>>) = 0;
  virtual void dumpJson(const std::map<Function *, std::vector<Function *>>,
                std::string filePath,const std::list<Function *> reverseTopologicalSort)=0;
  virtual std::map<Function *, std::vector<Function *>> loadJson(std::string filePath, llvm::Module &module, std::list<Function *> &reverseTopologicalSort) =0;
};
