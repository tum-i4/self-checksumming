#include <stdio.h>
int gv =0;
int f1(int a){
	if(a==2){
		printf("a is 2");
	}
        if(a>0){
                gv += 1;
		return 5;
        } else {
		gv+=2;
                return 10;
        }
}
int main(int argc, const char** argv) {
	int num=2;
	int c= 3;//10;
	//c+=10+10;
	//scanf("%i", &num);
	num += f1(num);
	//scanf("%i",&c);
	printf("%i\n", num);
	return 0;
}






