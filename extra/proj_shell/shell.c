#include <stdio.h> // fgets gets perror fopne
#include <string.h> // strtok strcmp strchr
#include <unistd.h> //execvp
#include <sys/wait.h> //wait
#include <stdlib.h> //EXIT_FAILURE EXIT_SUCCESS
#define LEN 1024

pid_t pid;
int status;

int parsing(char *deli, char *str, char *co[]){
	char *ptr = strtok(str, deli);
	int cnt =0;
	while(ptr!=NULL){
		co[cnt] = ptr;
		ptr = strtok(NULL,deli);
		cnt++;
	}
	return cnt;
} //전달 받은 deli로 str을 split한 다음 co에 저장 

void exec(char *command[]){
	if(pid==0){
		if(execvp(command[0],command)<0){
	 		perror("exec fail\n");
	 		kill(getpid(),SIGILL); 
		}
	}
}//command로 전달받은 인자를 execvp로 보내고 if문으로
//함수 실행을 확인 하고 실패할 경우 SIGILL발생 + kill
int main(int argc, char* argv[]){

	char input[LEN];
	pid_t pid;
	pid_t addr[LEN];
	int count, i;
	
	//interactive mode
	if(argc<2){
		printf("prompt>");
		while(fgets(input, sizeof(input),stdin)){
			input[strlen(input)-1] = '\0';
			if(strcmp(input,"quit")==0){
				exit(EXIT_SUCCESS);
			} else {
				char *command[100] = {'\0',};
				count = parsing(";",input, command);
				for(i=0;i<count;i++){
					addr[i] = pid = fork();
					if(pid<0) perror("fork fail\n");
					else if(pid == 0) {
						char *element[100] = {'\0',};
						parsing(" ",command[i],element);
						exec(element);
					}
				}

				for(i = 0; i < count; i++) {
					waitpid(addr[i], NULL, 0);
				}//parent프로세스는 child가 끝나기까지 기다린다
			}

			printf("prompt>");
			memset(input, '\0', sizeof(input)); //input값 초기화
		}
	}

	//batch file mode
	if(argc>1){
		FILE *fi = fopen(argv[1],"r");
		if(fi == NULL){
			perror("file open fail\n");
			exit(EXIT_FAILURE);
		}
		while(!feof(fi)) {
			if(fgets(input, sizeof(input), fi) == NULL) break;
			input[strlen(input) - 1] = '\0';
			printf("%s\n",input);
			if(strcmp(input, "quit") == 0) {
				exit(EXIT_SUCCESS);
			} else {
				char *command[100] = {'\0',};
				count = parsing(";",input, command);
				for(i=0;i<count;i++){
					addr[i] = pid = fork();
					if(pid<0) perror("fork fail\n");
					else if(pid == 0) {
						char *element[100] = {'\0',};
						parsing(" ",command[i],element);
						exec(element);
					}
				}
			}

			for(i=0; i< count ; i++){
				waitpid(addr[i],NULL,0);
			}

			memset(input, '\0', sizeof(input));
		}
		fclose(fi);
	}
	return 0;
}//인자가 2개 이상이면 batch모드로 1개면 interactive모드 실행
//';'로 한 line에 받은 명령어를 parsing하고 공백으로 옵션을 parsing