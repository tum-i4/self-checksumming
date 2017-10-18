#include <stdio.h>
void f3(){
	printf("f3 is called\n");
}
void f4(){
	printf("f4 is called\n");
}
void f1(){
        int a = 231;
        int b = a +21;
	printf("f1 is called %d\n", b);
//	f3();
}
void f2(){
        int a = 232;
        int b = 8912 + a;
	printf("f2 is called %d\n", b);
//	f4();
}
void show_high_score(void){
}
int main(int argc, const char** argv) {	
	int a = 0;
	scanf("%d", &a);
	if (a==0){
		f3();
	} else {
	        f4();
	}
	printf("main  is called\n");
	f1();
	f2();
	show_high_score();
	return 0;
}






