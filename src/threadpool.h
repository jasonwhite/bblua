/*
 * Copyright (c) 2016 Jason White
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

#include <functional>

#include <vector>
#include <queue>

/**
 * A thread pool.
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());

    // It should never be possible to copy a thread pool.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * Joins all threads after their current task (if any) is completed. Note
     * that `waitAll` should be called before destruction to ensure no work is
     * left undone.
     */
    ~ThreadPool();

    /**
     * Adds a new task to the end of the queue.
     */
    void enqueue(std::function<void()> task);

    /**
     * Wraps a task in a future and adds it to the end of the queue. This is
     * useful if you care about the result (but it has more overhead).
     */
    template<class F, class... Args>
    std::future<typename std::result_of<F(Args...)>::type>
    enqueue(F&& f, Args&&... args) {
        using packaged_task = std::packaged_task<typename std::result_of<F(Args...)>::type()>;

        std::shared_ptr<packaged_task> task(new packaged_task(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            ));

        auto res = task->get_future();

        enqueue([task](){ (*task)(); });

        return res;
    }

    /**
     * Blocks until all tasks in the queue have completed. This should be called
     * before destruction to ensure all work has been finished.
     */
    void waitAll();

private:
    /**
     * Takes the next task in the queue and runs it. Notify the main thread that a
     * task has completed. This is the main loop for each thread.
     */
    void worker();

    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _queue;

    size_t _tasksLeft;
    std::atomic_bool _stop;
    std::condition_variable _taskAvailableCond;
    std::condition_variable _waitCond;
    std::mutex _tasksLeftMutex;
    std::mutex _queueMutex;
};
