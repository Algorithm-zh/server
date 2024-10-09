#include<iostream>
#include"ThreadPool.h"
using namespace std;

//线程池初始化

ThreadPool::ThreadPool(int min, int max):m_maxThreads(max),m_minThreads(min),m_stop(false),m_exitNumber(0)
{
   
    m_idleThreads = m_curThreads = min;
    cout << "线程数量" << m_curThreads << endl;
    m_manager = new thread(&ThreadPool::manager, this);
    for(int i = 0; i < m_curThreads; i ++ )
    {
        thread t(&ThreadPool::worker, this);
        m_workers.insert(make_pair(t.get_id(), move(t)));//线程不可复制，所以用move转移过去
    }
}


//析构函数

ThreadPool::~ThreadPool()
{
    m_stop = true;
    m_condition.notify_all();
    for(auto &it: m_workers)
    {
        thread& tmp = it.second;
        if(tmp.joinable())//判断主线程和子线程是否关联
        {
            cout << tmp.get_id() << "线程退出" << endl;
            tmp.join();//阻塞主线程
        }
    }
    if(m_manager->joinable())m_manager->join();
    delete m_manager;
    
}

//消费者
void ThreadPool::worker()
{
    
    while(!m_stop.load())
    {
        function<void()> task = nullptr;
        {
            unique_lock<mutex> locker(m_queueMutex);
            while(!m_stop.load() && m_tasks.empty())//任务队列为空
            {
                m_condition.wait(locker);
                if(m_exitNumber.load() > 0)
                {
                    cout << "-----------线程任务结束，ID" << this_thread::get_id() << endl;
                    m_exitNumber -- ;
                    m_curThreads -- ;
                    unique_lock<mutex> lck(m_idsMutex);
                    m_ids.emplace_back(this_thread::get_id());
                    return ;
                }
            }

            if(!m_tasks.empty())
            {
                cout << "取出一个任务" << endl;
                task = move(m_tasks.front());
                m_tasks.pop();
            }
        }

        if(task)
        {
            m_idleThreads -- ;
            task();
            m_idleThreads ++ ;
        }
    }
}

//管理者线程
void ThreadPool::manager()
{
    while(!m_stop.load())
    {
        this_thread::sleep_for(chrono::seconds(2));
        int idle = m_idleThreads.load();
        int current = m_curThreads.load();
        if(idle > current / 2 && current > m_minThreads)//空闲的线程数大于当前线程数的一半，并且当前线程数大于最小线程数，则销毁一些线程（说明大部分线程都闲着）
        {
            m_exitNumber.store(2);//销毁两个线程
            m_condition.notify_all();//唤醒所有线程
            unique_lock<mutex> lck(m_idsMutex);//锁住对id的操作
            for(const auto& id : m_ids)
            {
                auto it = m_workers.find(id);//根据id找到对应的线程
                if(it != m_workers.end())
                {
                    cout << "########线程" << (*it).first << "被销毁########" << endl;
                    (*it).second.join();//阻塞线程
                    m_workers.erase(it);//从哈希表中删除线程
                }
            }
            m_ids.clear();
        }
        else if(idle == 0 && current < m_maxThreads)//空闲线程为0，并且当前线程数小于最大线程数，则添加一些线程
        {
            thread t(&ThreadPool::worker, this);
            cout << "线程" << t.get_id() << "创建成功" << endl;
            m_workers.insert(make_pair(t.get_id(), move(t)));
            m_curThreads ++ ;
            m_idleThreads ++ ;
        }
    }
}


void ThreadPool::addTask(function<void()> f)
{
    {
        lock_guard<mutex> locker(m_queueMutex);
        m_tasks.emplace(f);//在队列的末尾构造一个元素
    }
    m_condition.notify_one();//唤醒一个线程
}


void calc(int x, int y)
{
    int res = x + y;
    cout << "res = " << res << endl;
    this_thread::sleep_for(chrono::seconds(2));
}
// int calc(int x, int y)
// {
//     int res = x + y;
//     //cout << "res = " << res << endl;
//     this_thread::sleep_for(chrono::seconds(2));
//     return res;
// }
// int main()
// {
//     //普通线程池测试
//     ThreadPool pool(4);
//     for (int i = 0; i < 10; ++i)
//     {
//         auto func = bind(calc, i, i * 2);
//         pool.addTask(func);
//     }
//     getchar();


// // 异步线程池测试
//     // ThreadPool pool(4);
//     // vector<future<int>> results;

//     // for(int i = 0; i < 10; ++i)
//     // {
//     //     results.emplace_back(pool.addTask(bind(calc, i, i * 2)));
//     // }

//     //     // 等待并打印结果
//     // for (auto&& res : results)
//     // {
//     //     cout << "线程函数返回值: " << res.get() << endl;
//     // }


//     return 0;
// }
