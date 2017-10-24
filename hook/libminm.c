#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <string.h>
static __thread char thread_name_buffer[17] = { 0 };

const char *thread_name(void)
{
	/* Try obtaining the thread name.
	 *** If this fails, we'll return a pointer to an empty string. */
	if (!thread_name_buffer[0])
		prctl(PR_GET_NAME, thread_name_buffer, 0L, 0L, 0L);
	return (const char *)thread_name_buffer;
}
/*int tetris[]= {-1,-1,106,-1,-1,107,-1,-1,108,-1,108,108,108,-1,108,-1,32,-1,-1,108,107,-1,107,-1,107,107,-1,107,-1,-1,108,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,108,-1,108,-1,-1,107,-1,107,-1,-1,108,-1,-1,32,-1,-1,-1,107,-1,107,-1,106,-1,106,-1,-1,106,-1,-1,108,-1,32,-1,-1,-1,-1,-1,107,-1,-1,-1,108,-1,32,-1,-1,-1,-1,-1,-1,-1,32,-1,-1,32,-1,-1,-1,-1,107,-1,107,-1,107,-1,-1,106,-1,106,-1,32,-1,-1,-1,-1,107,-1,108,108,108,-1,108,-1,32,-1,-1,108,108,-1,-1,108,32,-1,-1,-1,107,-1,-1,106,106,-1,106,-1,32,-1,-1,-1,32,-1,-1,108,-1,107,-1,106,-1,106,106,106,-1,106,-1,32,-1,-1,-1,-1,-1,-1,-1,-1,113}*/;
int getchar(void){
	int (*new_getchar)(void);
	int result;
	static int triedToAccessFile = 0;	
	static FILE *f = NULL;
	if (f == NULL && !triedToAccessFile) {
		triedToAccessFile=1;
		char *fileName=calloc(50,sizeof(char));
		strcat(fileName,"intercept_");
		strcat(fileName,thread_name());
		if( access( fileName, F_OK ) != -1 ) {
			f = fopen(fileName, "r");
			free(fileName);
			if (f == NULL) {
				printf("Error opening file!\n");
				exit(1);
			}
		}
	}
	if (f!=NULL){
		char ch;
		while (EOF!=fscanf(f,"%d%c", &result, &ch)){
			return result;
		}
		fclose(f);
	} else {
		new_getchar= dlsym(RTLD_NEXT,"getchar");
		result = new_getchar();
		return result;
	}
	/*static int counter=0;
	  new_getchar = dlsym(RTLD_NEXT, "getchar");
	  if(strstr(thread_name(),"tetris")!=NULL){
	  result = tetris[counter++];
	  }else {
	  new_getchar = dlsym(RTLD_NEXT, "getchar"); 
	  result = new_getchar();
	  }*/
	//	return result;
}
