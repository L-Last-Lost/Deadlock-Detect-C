#include "deadlock.h"

// 测试用例

pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx3 = PTHREAD_MUTEX_INITIALIZER;

void *thread_routine_1(void *arg){
	
	pthread_mutex_lock(&mtx1);
	
	sleep(1);
	pthread_mutex_lock(&mtx2);
	
	// printf("我成功获取到锁了1");
	pthread_mutex_unlock(&mtx2);
	pthread_mutex_unlock(&mtx1);
}


void *thread_routine_2(void *arg){
	
	pthread_mutex_lock(&mtx2);
	
	sleep(1);
	pthread_mutex_lock(&mtx3);
	
	// printf("我成功获取到锁了2");
	pthread_mutex_unlock(&mtx3);
	pthread_mutex_unlock(&mtx2);
}

void *thread_routine_3(void *arg){
	
	pthread_mutex_lock(&mtx3);
	
	sleep(1);
	pthread_mutex_lock(&mtx1);
	
	
	// printf("我成功获取到锁了3");
	pthread_mutex_unlock(&mtx1);
	pthread_mutex_unlock(&mtx3);
}


int main() {
	// 两个关键函数
	init_hook();
	start_check();
	
	// 测试用例
	pthread_t threads[THREAD_NUM], tid1, tid2, tid3;
	
	pthread_create(&tid1, NULL, thread_routine_1, NULL);
	pthread_create(&tid2, NULL, thread_routine_2, NULL);
	pthread_create(&tid3, NULL, thread_routine_3, NULL);
	
	
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	
	free(tg);
	
	return 0;
}

