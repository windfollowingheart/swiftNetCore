#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include "../Logger.h"
using namespace std;

class ThreadPool
{
public:
    using MutexGuard = std::lock_guard<std::mutex>;
    using UniqueLock = std::unique_lock<std::mutex>;
    using Thread = std::thread;
    using ThreadID = std::thread::id;
    using Task = std::function<void()>;

    ThreadPool()
        : ThreadPool(Thread::hardware_concurrency())
    {
    }

    explicit ThreadPool(size_t maxThreads)
        : quit_(false),
          currentThreads_(0),
          idleThreads_(0),
          maxThreads_(maxThreads),
          maxTasks_(-1)
    {
    }

    explicit ThreadPool(size_t maxThreads, size_t maxTasks)
        : quit_(false),
          currentThreads_(0),
          idleThreads_(0),
          maxThreads_(maxThreads),
          maxTasks_(maxTasks)
    {
    }

    // disable the copy operations
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ~ThreadPool()
    {
        LOG_DEBUG("~ThreadPool()");
        {
            MutexGuard guard(mutex_);
            quit_ = true;
        }
        cv_.notify_all();

        for (auto &elem : threads_)
        {
            assert(elem.second.joinable());
            elem.second.join();
        }
    }

    void setMaxThreads(size_t maxThreads)
    {
        MutexGuard guard(mutex_);
        maxThreads_ = maxThreads;
    }

    void setMaxTasks(size_t maxTasks)
    {
        MutexGuard guard(mutex_);
        maxTasks_ = maxTasks;
    }

    size_t maxTasksNum() const
    {
        MutexGuard guard(mutex_);
        return maxTasks_;
    }

    template <typename Func, typename... Ts>
    auto submit(Func &&func, Ts &&...params)
        -> std::future<typename std::result_of<Func(Ts...)>::type>
    {
        auto execute = std::bind(std::forward<Func>(func), std::forward<Ts>(params)...);

        using ReturnType = typename std::result_of<Func(Ts...)>::type;
        using PackagedTask = std::packaged_task<ReturnType()>;

        auto task = std::make_shared<PackagedTask>(std::move(execute));
        auto result = task->get_future();

        MutexGuard guard(mutex_);
        assert(!quit_);

        // 判断任务队列是否超过任务队列容量
        if (maxTasks_ != -1 && tasks_.size() >= maxTasks_)
        {
            // 丢弃策略：直接丢弃新提交的任务
            // std::cout << "[INFO] task queue is full, drop new task" << std::endl;
            LOG_ERROR("task queue is full, drop new task");
            return result;
        }

        tasks_.emplace([task]()
                       { (*task)(); });

        if (idleThreads_ > 0)
        {
            cv_.notify_one();
        }
        else if (currentThreads_ < maxThreads_)
        {
            Thread t(&ThreadPool::worker, this);
            assert(threads_.find(t.get_id()) == threads_.end());
            threads_[t.get_id()] = std::move(t);
            ++currentThreads_;
        }

        return result;
    }

    size_t threadsNum() const
    {
        MutexGuard guard(mutex_);
        return currentThreads_;
    }

    // 获取任务数量
    size_t tasksNum() const
    {
        MutexGuard guard(mutex_);
        return tasks_.size();
    }

    // 获取空闲线程数量
    size_t idleThreadsNum() const
    {
        MutexGuard guard(mutex_);
        return idleThreads_;
    }

    // 获取最大线程数量
    size_t maxThreadsNum() const
    {
        MutexGuard guard(mutex_);
        return maxThreads_;
    }

private:
    void worker()
    {
        while (true)
        {
            Task task;
            {
                UniqueLock uniqueLock(mutex_);
                ++idleThreads_; // 还没开始执行任务，空闲线程+1
                auto hasTimedout = !cv_.wait_for(uniqueLock,
                                                 std::chrono::seconds(WAIT_SECONDS),
                                                 [this]()
                                                 {
                                                     return quit_ || !tasks_.empty();
                                                 });
                --idleThreads_; // 要开始执行任务或销毁线程，空闲线程-1 (后续只有两种结果，要么执行任务，要么销毁线程, 所以这里要-1)
                if (tasks_.empty())
                {
                    if (quit_)
                    {
                        --currentThreads_;
                        return;
                    }
                    if (hasTimedout)
                    {
                        --currentThreads_; // 相当于清理掉当前线程，即把当前进程减掉
                        joinFinishedThreads();
                        finishedThreadIDs_.emplace(std::this_thread::get_id()); // 线程不能直接清理自己
                        return;
                    }
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    void joinFinishedThreads()
    {
        while (!finishedThreadIDs_.empty())
        {
            auto id = std::move(finishedThreadIDs_.front());
            finishedThreadIDs_.pop();
            auto iter = threads_.find(id);

            assert(iter != threads_.end());
            assert(iter->second.joinable());

            iter->second.join();
            threads_.erase(iter);
        }
    }

    static constexpr size_t WAIT_SECONDS = 2;

    bool quit_;
    size_t currentThreads_;
    size_t idleThreads_;
    size_t maxThreads_;
    size_t maxTasks_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
    std::queue<ThreadID> finishedThreadIDs_;
    std::unordered_map<ThreadID, Thread> threads_;
};

constexpr size_t ThreadPool::WAIT_SECONDS;
