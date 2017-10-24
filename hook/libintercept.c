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
	 *         *** If this fails, we'll return a pointer to an empty string. */
	if (!thread_name_buffer[0])
		prctl(PR_GET_NAME, thread_name_buffer, 0L, 0L, 0L);
	return (const char *)thread_name_buffer;
}
int getchar(void){
	int (*new_getchar)(void);
	int result;
	new_getchar = dlsym(RTLD_NEXT, "getchar");
	//Intercept
	char *fileName=calloc(50,sizeof(char));
	strcat(fileName, "intercept_");
	strcat(fileName, thread_name());
	static	FILE *f = NULL;
	if(f==NULL){
		f = fopen(fileName, "w");	
	} else {
		f = fopen(fileName,"a");
	}
	if (f == NULL)
	{
		printf("Error opening file %s!\n",fileName);
		exit(1);
	}
	result = new_getchar();
	fprintf(f,"%d,",result);
	fclose(f);
	free(fileName);
	/*static FILE *f = fopen("intercept.txt", "r");
	  if (f == NULL) {
	  printf("Error opening file!\n");
	  exit(1);
	  }
	  do 
	  {
	 *code++ = (char)fgetc(f);
	 return *code
	 } while(*code != EOF);
	 fclose(f);*/

	return result;
}
