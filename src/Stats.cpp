#include "self-checksumming/Stats.h"
#include <numeric>
#include <iomanip>

using json = nlohmann::json;
void Stats::setNumberOfSensitiveInstructions(long value){
	this->numberOfSensitiveInstructions=value;
}

void Stats::addNumberOfProtectedInstructions(long value){
	this->numberOfProtectedInstructions += value;
}
void Stats::addNumberOfProtectedFunctions(int value){
	this->numberOfProtectedFunctions += value;
}
void Stats::setDesiredConnectivity(int  value){
	this->desiredConnectivity = value;
}
void Stats::setAvgConnectivity(double value){
	this->avgConnectivity = value;
}	
void Stats::setStdConnectivity(double value){
	this->stdConnectivity = value;
}
void Stats::addNumberOfGuards(int value){
	this->numberOfGuards += value;
}
void Stats::addNumberOfGuardInstructions(int value){
	this->numberOfGuardInstructions += value;
}
void Stats::calculateConnectivity(std::vector<int> v){
	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	double mean = sum / v.size();

	std::vector<double> diff(v.size());
	std::transform(v.begin(), v.end(), diff.begin(),
			std::bind2nd(std::minus<double>(), mean));
	double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
	double stdev = std::sqrt(sq_sum / v.size());
	this->avgConnectivity=mean;
	this->stdConnectivity=stdev;
}
void Stats::dumpJson(std::string filePath){
	json j;
	j["numberOfSensitiveInstructions"] = this->numberOfSensitiveInstructions;
	j["numberOfProtectedInstructions"] = this->numberOfProtectedInstructions;
	j["numberOfProtectedFunctions"] = this->numberOfProtectedFunctions;
	j["avgConnectivity"] = this->avgConnectivity;
	j["stdConnectivity"] = this->stdConnectivity;
	j["numberOfGuards"] = this->numberOfGuards;
	j["numberOfGuardInstructions"] = this->numberOfGuardInstructions;
	j["desiredConnectivity"] = this->desiredConnectivity;
	std::cout << j.dump(4) << std::endl;
	std::ofstream o(filePath);
	o << std::setw(4) << j << std::endl;
}


