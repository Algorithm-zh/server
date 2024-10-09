#pragma once
#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <functional>
#include <map>
#include <future>

using namespace std;


class ThreadPool {
public:
    //创建线程池并初始化
    ThreadPool(int min = 4, int max = thread::hardware_concurrency());
    //销毁线程池
    ~ThreadPool();

    //给线程池添加任务
    void addTask(function<void()> f);

    //异步线程池实现
    //技术涉及 1.共享指针，2.可调用对象绑定器，3.异步线程实现，4.可变参数模版，5.自动类型推导，6. lambda表达式，7.泛型编程
    // template<typename F, typename... Args>//。。。表示接受不定量的参数
    // auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>
    // {
    //     using returnType = typename result_of<F(Args...)>::type;
    //     auto task = make_shared<packaged_task<returnType()>>
    //     (
    //         bind(forward<F>(f), forward<Args>(args)...)
    //     );
    //     future<returnType> res = task->get_future();
    //     {
    //         unique_lock<mutex> lock(m_queueMutex);
    //         m_tasks.push([task](){(*task)();});
    //     }
    //     m_condition.notify_one();
    //     return res;
    // }
    

private:
    //工作的线程（消费者线程）任务函数
    void worker();
    //管理者线程任务函数
    void manager();

    thread* m_manager;//管理者线程
    map<thread::id, thread> m_workers;//工作的线程
    vector<thread::id> m_ids;//线程id
    int m_minThreads;//最小线程数
    int m_maxThreads;//最大线程数
    atomic_bool m_stop;//线程池是否停止
    atomic_int m_curThreads;//当前线程数
    atomic_int m_idleThreads;//空闲线程数
    atomic_int m_exitNumber;//需要退出的线程数
    queue<function<void()>> m_tasks;
    mutex m_idsMutex;//线程id的互斥锁
    mutex m_queueMutex;//任务队列的互斥锁

    condition_variable m_condition;//线程池的条件变量
    
};