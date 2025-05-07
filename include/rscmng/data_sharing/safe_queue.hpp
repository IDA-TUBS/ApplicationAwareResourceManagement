// Copyright (C) 2025 IDA 
//
// This file is part of a project licensed under the GNU Lesser General Public License v3.0.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#ifndef SAFE_QUEUE_h
#define SAFE_QUEUE_h


#include <queue>
#include <mutex>
#include <condition_variable>


/**
 * @brief A thread safe queue
 * 
 * @tparam T the data type to be stored in the queue
 */
template <class T>
class SafeQueue
{
    public:
    
    /**
     * @brief Construct a new (empty) Safe Queue object
     * 
     */
    SafeQueue(void)
    : safe_queue()
    , queue_lock()
    , queue_event()
    {}

    /**
     * @brief Destroy the Safe Queue object (default)
     * 
     */
    ~SafeQueue(void)
    {}

    /**
     * @brief Thread safe wrapper for std::queue push. A thread waiting on dequeue is notified after the enqueue operation
     * 
     * @param value the value to be enqueued
     */
    void enqueue(T value)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        safe_queue.push(value);
        queue_event.notify_one();
    }

    /**
     * @brief Get the "front"-element. If the queue is empty, wait till an element is available
     * 
     * @return T The dequeued value
     */
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(queue_lock);
        while(safe_queue.empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            queue_event.wait(lock);
        }
        T val = safe_queue.front();
        safe_queue.pop();
        return val;
    }

    /**
     * @brief Thread safe wrapper for std::queue empty()
     * 
     * @return true empty queue
     * @return false non empty queue
     */
    bool empty()
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        return safe_queue.empty();
    }

    protected:
    
    /**
     * @brief The underlying queue which is accessed via the queue_lock (mutex)
     * 
     */
    std::queue<T> safe_queue;

    /**
     * @brief The mutex controlling the concurrent queue access
     * 
     */
    mutable std::mutex queue_lock;
    
    /**
     * @brief Condition variable used for an enqueue event. The blocking dequeue call waits for an enqueue event if the queue is empty.
     * 
     */
    std::condition_variable queue_event;
};



#endif




