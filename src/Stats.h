#include "json.hpp"
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
class Stats{
	private:
		long numberOfProtectedInstructions=0;
		long numberOfSensitiveInstructions=0;
		int numberOfProtectedFunctions=0;
		double avgConnectivity=0;
		double stdConnectivity=0;
		int numberOfGuards=0;
		int numberOfGuardInstructions=0;
		int desiredConnectivity=1;
	public:
		void setNumberOfSensitiveInstructions(long);
		void calculateConnectivity(std::vector<int>);
		void setDesiredConnectivity(int);
		void addNumberOfProtectedInstructions(long);
		void addNumberOfProtectedFunctions(int);
		void setAvgConnectivity(double);
		void setStdConnectivity(double);
		void addNumberOfGuards(int);
		void addNumberOfGuardInstructions(int);
		void dumpJson(std::string fileName);
};
