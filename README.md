死锁检测的接口，可以直接使用，c/c++

创建了一个定时触发的线程，每五秒执行一次check_dead_lock 来检测是否有环，也就是是否存在死锁

用到了hook， dfs， 原子操作__asm__， 

关键是task_graph
其中list 就是 线程列表， 每个节点是一个线程， 他连接的链表就是他请求的资源
locklist 就是 map， 存储现在每个锁分配给了哪个线程


![image](https://github.com/user-attachments/assets/1440ad3b-855a-4d7d-bdc3-3e8c73c32c39)
