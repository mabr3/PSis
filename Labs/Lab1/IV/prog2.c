#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


int main(){
	int a;
	void * handler;
	void (* funcao_1)();
	void (* funcao_2)();
	char *string;
	printf("What version of the functions you whant to use?\n");
	printf("\t1 - Normal    (test1)\n");
	printf("\t2 - Optimized (test2)\n");
	scanf("%d", &a);


	if (a == 1){
		/* load library test1 */


		handler = dlopen ("./test1.so", RTLD_NOW);
		funcao_1 = dlsym(handler,"func_1");
		/*string = dlerror();printf("%s\n",string); -- ver erros */
		printf("running the normal versions from \n");
		funcao_1();

	}else{
		if(a== 2){
			/* load library test2 */

			handler = dlopen ("./test2.so", RTLD_NOW);
			funcao_2 = dlsym(handler,"func_2");
			/*string = dlerror();printf("%s\n",string); -- ver erros */
			printf("running the normal versions\n");
			funcao_2();
		}else{
			printf("Not running anything\n");
			exit(-1);
		}
	}
	/* call func_1 from whichever library was loaded */
	/* call func_2 from whichever library was loaded */
	exit(0);


}
