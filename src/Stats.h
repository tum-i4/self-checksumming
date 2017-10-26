#include "json.hpp"
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
class Stats{
	private:
		int numberOfProtectedInstructions=0;
		int numberOfProtectedFunctions=0;
		double avgConnectivity=0;
		double stdConnectivity=0;
		int numberOfGuards=0;
		int numberOfGuardInstructions=0;
		int desiredConnectivity=1;
	public:
		void calculateConnectivity(std::vector<int>);
		void setDesiredConnectivity(int);
		void addNumberOfProtectedInstructions(int);
		void addNumberOfProtectedFunctions(int);
		void setAvgConnectivity(double);
		void setStdConnectivity(double);
		void addNumberOfGuards(int);
		void addNumberOfGuardInstructions(int);
		void dumpJson(std::string fileName);
};
