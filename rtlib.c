#include <stdio.h>
void logop(int i) {
	printf("computed: %i\n", i);
}
long hash =0;
void hashMe(int i) {
	printf("adding hash %i\n", i);
	hash +=i;
}

void guardMe(long address, long length){

	const char *beginAddress = (const char *)address;
	long visited = 0;
	long hash = 0;
	while (visited < length) {
		hash ^= *beginAddress++;
		++visited;
	}
	printf("new %x\n",hash);
}
//void dbghashMe(int i, std::string valueName){
//	printf("adding hash %s %i\n",valueName, i);
//        hash +=i;
//}
void logHash() {
	printf("final hash: %ld\n", hash);
}
