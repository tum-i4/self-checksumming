#include "self-checksumming/DAGCheckersNetwork.h"
#include <iostream>
int main() {

  DAGCheckersNetwork network;
  std::vector<int> actualConnectivity;
  network.constructAcyclicCheckers(2, 2, actualConnectivity);
  /*for(int i:actualConnectivity){
          std::cout << i <<",";
  }*/
}
