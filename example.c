#include <stdio.h>
void f1(){
	printf("f1 is called\n");
}
void f2(){
	printf("f2 is called\n");
}
int main(int argc, const char** argv) {	
	printf("main  is called\n");
	f1();
	f2();
	return 0;
}






