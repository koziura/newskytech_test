/*
 *
1.Определите и перечислите факторы, влияющие на производительность.

2.Минимизируйте количество системных вызовов, попытайтесь ускорить работу программы.
Перечислить какие вызовы были удалены и почему? Предоставьте исправленный вариант
программы. Предоставьте методологию измерений и численные показатели ускорения.

3.Обеспечьте одновременную работу нескольких экземпляров данного приложения с одной
SQLITE-базой не полагаясь на системный вызов fcntl (например, если в ядре системы
некорректно реализованы блокировки файлов). Внесите исправления в оригинальный код
программы. Задокументируйте использованные методы.

*/
/*
* gcc main.c -o test -lpthread -luuid -lsqlite3
*/
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <sqlite3.h>
#include <time.h>

#include<sys/ipc.h>
#include<sys/shm.h>

int shmid;
int shmkey = IPC_PRIVATE;
char *shmpointer = NULL;

sqlite3* db;
static int callback(void* NotUsed, int argc, char** argv, char** azColName)
{
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
}
static void* thread_routine(void* arg)
{
	uuid_t uuid;
	char str[37];
	char* zErrMsg = 0;
	int rc;
	int i;
	char sql[100];

	clock_t start, end;
	double cpu_time_used;

	for (i = 0; i < 1000000; i++) {
		start = clock();
		//uuid_generate(uuid);
		//uuid_unparse(uuid, str);
		printf("[tid = %lu] thread_routine1 i = %i UUID:[%s]\n", pthread_self(), (int)arg, str);
		//snprintf(sql, 100, "INSERT INTO UUIDS (ID,UUID) VALUES ('%s', %i ); ", str, (int)arg);
		snprintf(sql, 100, "INSERT INTO UUIDS (UUID) VALUES ('%i');", (int)arg);
		rc = SQLITE_BUSY;

		for (;;) {
			if ( *shmpointer != 0 ) {
				usleep(100);
				continue;
			}

			*shmpointer =
			rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

			if ( rc == SQLITE_BUSY ) {
				usleep(100);
				continue;
			}



			break;
		}

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		} else {
			printf("Records created successfully\n");
		}
		end = clock();
		cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		printf("[cpu timer = %f]\n", cpu_time_used);
	}
	pthread_exit((void*)0);
	return 0;
}
int main(int argc, const char* argv[])
{
	int res = 0;
	int rc = 0;
	char* sql = 0;
	char* zErrMsg = 0;
	pthread_t thread1;
	pthread_t thread2;
	pthread_attr_t attr;
	res = pthread_attr_init(&attr);
	/* res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); */
	/* unlink("test.db"); */
	rc = sqlite3_open("test.db", &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return EXIT_FAILURE;
	} else {
		printf("Opened database successfully\n");
	}

//	sql = "CREATE TABLE UUIDS("
//		  "ID CHAR(50) PRIMARY KEY NOT NULL,"
//		  "UUID INT);";

	sql = "CREATE TABLE IF NOT EXISTS UUIDS ( "
			  "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
			  "UUID INT );";

	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		printf("Table created successfully\n");
	}

	shmid = shmget(shmkey, 2, IPC_CREAT | IPC_EXCL | IPC_EXCL);
	shmpointer = shmat(shmid, NULL, 0);
	shmpointer[0] = shmpointer[0]+1;

	if ((res = pthread_create(&thread1, &attr, thread_routine, 1) != 0)) {
		printf("pthread_create failure failure: %d errno: %d:%s\n", res, errno, strerror(errno));
	}

	if ((res = pthread_create(&thread2, &attr, thread_routine, 2) != 0)) {
		printf("pthread_create failure failure: %d errno: %d:%s\n", res, errno, strerror(errno));
	}

	shmpointer[0] -= 1;

	shmdt(&shmpointer);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	sqlite3_close(db);
	return EXIT_SUCCESS;
}
