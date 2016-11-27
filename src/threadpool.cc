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
#include "threadpool.h"

ThreadPool::ThreadPool(size_t threads) :
    _threads(threads), _tasksLeft(0), _stop(false)
{
    // Initialize worker threads.
    for (size_t i = 0; i < threads; ++i) {
        _threads[i] = std::move(std::thread([this] { this->worker(); }));
    }
}

ThreadPool::~ThreadPool() {
    _stop = true;

    // Wake up all threads waiting for a new task.
    _taskAvailableCond.notify_all();

    for (auto& x : _threads) {
        if (x.joinable())
            x.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _queue.emplace(task);
    }

    {
        std::lock_guard<std::mutex> lock(_tasksLeftMutex);
        ++_tasksLeft;
    }

    _taskAvailableCond.notify_one();
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(_tasksLeftMutex);
    if (_tasksLeft > 0) {
        _waitCond.wait(lock, [this] {
                return _tasksLeft == 0;
                });
    }
}

void ThreadPool::worker() {
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            if (_stop) return;

            // Wait for a task if we don't have any.
            _taskAvailableCond.wait(lock, [this] {
                    return _queue.size() > 0 || _stop;
                    });

            if (_stop) return;

            // Get task from the queue
            task = std::move(_queue.front());
            _queue.pop();
        }

        task();

        {
            std::lock_guard<std::mutex> lock(_tasksLeftMutex);
            --_tasksLeft;
        }

        _waitCond.notify_one();
    }
}
