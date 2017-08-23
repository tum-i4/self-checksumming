#include <stdint.h>
#include <stdio.h>
void guardMe(unsigned int address,unsigned short length){

	const unsigned int *beginAddress = (const unsigned int *)address;
	unsigned int visited = 0;
	unsigned int hash = 0;
	//TODO: Length need to be divided by the size of unsigned int that we are reading each time, otherwise it falls out of the scope
	while (visited < length) {
		hash ^= *beginAddress++;
		++visited;
	}
	printf("new %zu\n",hash);
}
//void dbghashMe(int i, std::string valueName){
//	printf("adding hash %s %i\n",valueName, i);
//        hash +=i;
//}
void logHash() {
	//printf("final hash: %ld\n", hash);
}
