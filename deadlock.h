#ifndef _DEAD_LOCK_H_
#define _DEAD_LOCK_H_

// 引入 GNU 扩展和动态链接库
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

// 线程和图的相关定义
#define THREAD_NUM 10         // 线程数
#define THREAD_MAX 10         // 最大线程数
#define MAX 100               // 图的最大顶点数

// 使用无符号长整型表示ID
typedef unsigned long int uint64;

// pthread互斥锁函数类型定义
typedef int (*pthread_mutex_lock_t)(pthread_mutex_t *mutex);
pthread_mutex_lock_t pthread_mutex_lock_f; // 互斥锁函数指针

typedef int (*pthread_mutex_unlock_t)(pthread_mutex_t *mutex);
pthread_mutex_unlock_t pthread_mutex_unlock_f; // 互斥解锁函数指针

// 类型定义：表示线程和资源
enum Type { PROCESS, RESOURCE };

// 资源类型结构体
struct source_type {
	uint64 id;               // 资源或线程ID
	enum Type type;          // 类型：线程或资源
	uint64 lock_id;          // 锁ID
	int degress;             // 锁的使用度数
};

// 图顶点结构体
struct vertex {
	struct source_type s;    // 顶点的资源信息
	struct vertex* next;     // 下一个相邻顶点指针
};

// 任务图结构体，用于检测死锁
struct task_graph {
	struct vertex list[MAX]; // 顶点列表
	int num;                 // 顶点数
	struct source_type locklist[MAX]; // 锁列表
	int lockidx;             // 锁的索引
	pthread_mutex_t mutex;   // 互斥锁
};

struct task_graph *tg = NULL;   // 全局任务图指针
int path[MAX + 1];              // 记录路径
int visited[MAX];               // 记录顶点是否被访问
int k = 0;                      // 路径索引
int deadlock = 0;               // 死锁标记

// 打印锁列表
void print_locklist(void) {
	printf("print_locklist: \n");
	printf("-------------------------\n");
	for (int i = 0; i < tg->lockidx; i++)
		printf("threadid : %ld, lockid: %ld\n", tg->locklist[i].id, tg->locklist[i].lock_id);
	printf("--------------------------\n\n");
}

// 创建新顶点
struct vertex *create_vertex(struct source_type type) {
	struct vertex* tex = (struct vertex*)malloc(sizeof(struct vertex));
	tex->s = type;
	tex->next = NULL;
	return tex;
}

// 查找顶点
int search_vertex(struct source_type type) {
	for (int i = 0; i < tg->num; i++) {
		if (tg->list[i].s.type == type.type && tg->list[i].s.id == type.id)
			return i;
	}
	return -1;
}

// 添加顶点
void add_vertex(struct source_type type) {
	if (search_vertex(type) == -1) {
		tg->list[tg->num].s = type;
		tg->list[tg->num].next = NULL;
		tg->num++;
	}
}

// 添加边（建立依赖关系）
int add_edge(struct source_type from, struct source_type to) {
	add_vertex(from);
	add_vertex(to);
	struct vertex *v = &(tg->list[search_vertex(from)]);
	while (v->next != NULL) {
		v = v->next;
	}
	v->next = create_vertex(to);
}

// 验证是否存在边（是否已建立依赖关系）
int verify_edge(struct source_type i, struct source_type j) {
	if (tg->num == 0) return 0;
	int idx = search_vertex(i);
	if (idx == -1) return 0;
	struct vertex *v = &(tg->list[idx]);
	while (v != NULL) {
		if (v->s.id == j.id)
			return 1;
		v = v->next;
	}
	return 0;
}

// 移除边（解除依赖关系）
int remove_edge(struct source_type from, struct source_type to) {
	int idxi = search_vertex(from);
	int idxj = search_vertex(to);
	if (idxi != -1 && idxj != -1) {
		struct vertex *v = &tg->list[idxi];
		struct vertex *remove;
		while (v->next != NULL) {
			if (v->next->s.id == to.id) {
				remove = v->next;
				v->next = v->next->next;
				free(remove);
				break;
			}
			v = v->next;
		}
	}
}

// 打印死锁路径
void print_deadlock(void) {
	int i = 0;
	printf("deadlock :");
	for (int i = 0; i < k - 1; i++) {
		printf("%ld --> ", tg->list[path[i]].s.id);
	}
	printf("%ld\n", tg->list[path[i]].s.id);
}

// 深度优先搜索检测死锁
int DFS(int idx) {
	struct vertex* ver = &tg->list[idx];
	if (visited[idx] == 1) {
		path[k++] = idx;
		print_deadlock();
		deadlock = 1;
		return 0;
	}
	visited[idx] = 1;
	path[k++] = idx;
	while (ver->next != NULL) {
		DFS(search_vertex(ver->next->s));
		k--;
		ver = ver->next;
	}
	return 1;
}

// 循环检测
int search_for_cycle(int idx) {
	struct vertex *ver = &tg->list[idx];
	visited[idx] = 1;
	k = 0;
	path[k++] = idx;
	while (ver->next != NULL) {
		for (int i = 0; i < tg->num; i++) {
			if (i == idx) continue;
			visited[i] = 0;
		}
		for (int i = 1; i <= MAX; i++) {
			path[i] = -1;
		}
		k = 1;
		DFS(search_vertex(ver->next->s));
		ver = ver->next;
	}
}

// 检查死锁
void check_dead_lock(void) {
	deadlock = 0;
	for (int i = 0; i < tg->num; i++) {
		if (deadlock == 1) break;
		search_for_cycle(i);
	}
	if (deadlock == 0)
		printf("no deadlock\n");
}

// 线程例程，定期检查死锁
static void* thread_routine(void *args) {
	while (1) {
		sleep(5);
		print_locklist();
		check_dead_lock();
	}
}

// 启动死锁检查
void start_check(void) {
	tg = (struct task_graph*)malloc(sizeof(struct task_graph));
	tg->num = 0;
	tg->lockidx = 1;
	pthread_t tid;
	pthread_create(&tid, NULL, thread_routine, NULL);
}

// 查找锁
int search_lock(uint64 lock){
	for(int i = 0; i < tg->lockidx; i++){
		if(tg->locklist[i].lock_id == lock)
			return i;
	}
	return -1;
}

// 查找空位置插入新锁
int search_empty_lock(uint64 lock){
	for(int i = 0; i < tg->lockidx; i++){
		if(tg->locklist[i].lock_id == 0)
			return i;
	}
	
	return  -1;
}

// 原子操作，避免多线程导致lockidx 自增时出错
int inc(int *value, int add){
	int old;
	
	__asm__ volatile(
		"lock;xaddl %2, %1;"
		: "=a"(old)
		: "m"(*value), "a"(add)
		: "cc", "memory"
		);
	
	return old;
}


// 锁前检查
void lock_before(pthread_t thread_id, uint64 lockaddr){
	int idx = 0;
	for(idx = 0; idx < tg->lockidx; idx++){
		if(tg->locklist[idx].lock_id == lockaddr){
			// print_locklist();
			
			struct source_type from;
			from.id = thread_id;
			from.type = PROCESS;
			add_vertex(from);
			
			struct source_type to;
			to.id = tg->locklist[idx].id;
			tg->locklist[idx].degress++;
			to.type = PROCESS;
			add_vertex(to);
			
			
			if(!verify_edge(from, to))
				add_edge(from, to);
		} 
	}
	
}

// 锁后放入tg中， 使算法成型
void lock_after(pthread_t thread_id, uint64 lockaddr){
	int idx = 0;
	if(-1 == (idx = search_lock(lockaddr))){
		int eidx = search_empty_lock(lockaddr);
		
		tg->locklist[eidx].id = thread_id;
		tg->locklist[eidx].lock_id = lockaddr;
		
		inc(&tg->lockidx, 1);
		
		// printf("lockidx : %d\n", tg->lockidx - 1);
		
	}else{
		
		struct source_type from;
		from.id = thread_id;
		from.type = PROCESS;
		
		struct source_type to;
		to.id = tg->locklist[idx].id;
		tg->locklist[idx].degress --;
		to.type = PROCESS;
		
		if(verify_edge(from, to))
			remove_edge(from, to);
		
		tg->locklist[idx].id = thread_id;
	}
}

// 减锁后处理图
void unlock_after(pthread_t thread_id, uint64 lockaddr){
	
	int idx = search_lock(lockaddr);
	
	if(tg->locklist[idx].degress == 0){
		tg->locklist[idx].id = 0;
		tg->locklist[idx].lock_id = 0;
	}
}




int pthread_mutex_lock(pthread_mutex_t *mutex){
	
	pthread_t self_id = pthread_self();
	
	lock_before(self_id, (uint64)mutex);
	pthread_mutex_lock_f(mutex);
	
	lock_after(self_id, (uint64)mutex);
	
	// printf("pthread_mutex_lock --> self_id %ld, mtx: %ld\n", self_id, (uint64)mutex);
}



int pthread_mutex_unlock(pthread_mutex_t *mutex){
	
	pthread_t self_id = pthread_self();
	
	pthread_mutex_unlock_f(mutex);
	
	// printf("释放！");
	unlock_after(self_id, (uint64)mutex);
	
	// printf("pthread_mutex_unlock --> self_id %ld, mtx: %ld\n", self_id, (uint64)mutex);
}

// 钩住mutex的加锁减锁操作，新增死锁检测函数
static int init_hook(void){
	pthread_mutex_lock_f = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	
	pthread_mutex_unlock_f = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	
}

#endif
