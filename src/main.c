#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "string.h"

const int MaxBuf = 4096;

int main(){
	char template[] = "/tmp/tmpXXXXXX";
	int desc = mkstemp(template);					// возвращается дескриптор созданного временного файла
	if(desc < 0){
		perror("Tmp file cannot be created.\n");
		return -6;
	}
	if(ftruncate(desc, sizeof(float)) < 0){				// устанавливает длину файла в 1 флот
		perror("Tmp file can not filled.\n");
		return -7;
	}
	char* file_name;
	file_name = malloc(sizeof(char)*MaxBuf);				// выделяется временная память для имени файла
	if (read(0, file_name, MaxBuf) < 0){				// читается имя файла со стандартного ввода
		perror("Cannot read a name of the file.\n");
		return 1;
	}
	int l1 = strlen(file_name);					// находим длину строки
	file_name[l1 - 1] = '\0';					// добавляется \0 т.к. он должен быть в конце массива чаров
	fflush(stdout);							// очищается стандартный вывод
	int file = open(file_name, O_RDONLY);				// открывает файл на чтение, open() возвращает дескриптор (0 - стандартный ввод, 1 - стандартный вывод, 
	if(file < 0){							// прочие - какие-то файловые дескрипторы, -1 - всё плохо)
		perror("File cannot be opened.");
		free(file_name);
		return -5;
	}
	int pid = fork();						// создаёт новый процесс и запоминает индентификатор процесса
	if(pid == -1){
		perror("Cannot create a fork.");
		return -1;
	}
	if(pid == 0){							// дочерний процесс получает идентификатор 0
		if(dup2(file, 0) < 0){					// перенаправляет стандартный поток ввода
			perror("Cannot dup fd[0] to stdin.");
			return -5;
		}
		if(execl("child", "child", template, NULL) == -1){	// запуск дочерней программы
			perror("Execl child problem");
			return -7;
		}
	}
	else{								// процесс-родитель получает идентификатор > 0
		int status;
		wait(&status);						// ожидает изменение статуса дочернего процесса
		if(WEXITSTATUS(status)){
			return -1;
		}
		float* fd = mmap(0, sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, desc, 0);		// проецирует файл в оперативную память
		if(fd == MAP_FAILED){
			perror("Mmap error.\n");
			return -5;
		}
		printf("%f\n", fd[0]);
		if(munmap(fd, sizeof(float)) < 0){			// снимает отражение файла из оперативной памяти
			perror("Munmap error.");
			return -6;
		}
	}
	return 0;
}
