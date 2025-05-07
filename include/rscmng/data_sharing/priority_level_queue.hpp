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


#ifndef PRIORITY_LEVEL_QUEUE_h
#define PRIORITY_LEVEL_QUEUE_h


#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <array>


#include <rscmng/attributes/rm_parameters.hpp>
#include <rscmng/log.hpp>


/**
 * @brief Threadsafe priority queue 
 * 
 * @tparam T type of the queued objects
 */
template <class T>
class SharedPriorityLevelQueue
{
    public:

    /**
     * @brief Construct a new Shared Priority Level Queue object
     * 
     * @param prio_l int Number of priority levels used
     */
    SharedPriorityLevelQueue(int prio_l)
    :   priority_queue(prio_l),
        queue_lock(),
        enqueue_event(),
        priority_levels(prio_l)
    {}

    /**
     * @brief Destroy the Shared Priority Level Queue object
     * 
     */
    ~SharedPriorityLevelQueue(void)
    {}

    /**
     * @brief Check if the whole queue is empty
     * 
     * @return true the queues for all priority levels are empty
     * @return false the queue of at least one priority level is not empty 
     */
    bool empty()
    {
        bool queue_state = true;
        for(int i=0; i<priority_levels; i++)
        {
            queue_state = queue_state && priority_queue[i].empty();
        }
        return queue_state;
    }

    /**
     * @brief Check if the queue for a specific priority level is empty
     * 
     * @param priority_level int The priority level to check
     * @return true The queue for the specified level is empty
     * @return false The queue for the specified level is not empty
     */
    bool empty(int priority_level)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        return priority_queue[priority_level].empty();
    }

    /**
     * @brief Chechk if any of the queues for the different priority levels is not empty
     * 
     * @return int The priority level which is not empty
     */
    int prio_empty()
    {
        int prio_index = 0;
        for(; prio_index<priority_levels; prio_index++)
        {
            if(!priority_queue[prio_index].empty())
            {
                break; 
            }
        }
        return prio_index;
    }


    int prio_empty(int frags)
    {
        int prio_index = 0;
        for(; prio_index<priority_levels; prio_index++)
        {
            if(priority_queue[prio_index].size() >= frags)
            {
                break; 
            }
        }
        return prio_index;
    }

    /**
     * @brief Read the sizes of the priority level queues
     * 
     * @return size_t* pointer to an array containing the sizes of the different priority queues
     */
    size_t* size()
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        size_t queue_size[priority_levels];
        for(int i=0; i<priority_levels; i++)
        {
            queue_size[i] = priority_queue[i].size();
        }

        return queue_size;
    }

    /**
     * @brief Read the size of the queue for a specific priority level
     * 
     * @param priority_level The priority level to check
     * @return size_t The size of the queue for the specified priority level
     */
    size_t size(int priority_level)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        return priority_queue[priority_level].size();
    }

    /**
     * @brief Enqueues the given data object into the sub queue with the specified priority level
     * 
     * @param value The data object to enqueue
     * @param priority_level the priority of the data object
     */
    void enqueue(T value, int priority_level)
    {

        std::lock_guard<std::mutex> lock(queue_lock);
        priority_queue[priority_level].push(value);
        enqueue_event.notify_one();
    }

    /**
     * @brief Enqueues an array of data objects. The array index defines the priority level of the respective data object
     * 
     * @param values the array of data objects to enqueue, where the array index defines the priority level
     */
    void enqueue(T* values)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        for(int i=0; i<priority_levels; i++)
        {
            priority_queue[i].push(values[i]);
        }
        enqueue_event.notify_one();
    }

    /**
     * @brief Enqueues and vector of data objects. The vector for storing a complete sample before enqueue the associated fragments.
     * Using this version of enqueue, a store and forward approach can be implemented.
     * 
     * @param values std::vector<T> containing the fragments of a data sample
     * @param priority_level the priority of the sample
     */
    void enqueue(std::vector<T> values, int priority_level)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        for(auto it = values.begin(); it != values.end(); it++)
        {
            priority_queue[priority_level].push(*it);
        }
        enqueue_event.notify_one();
    }

    /**
     * @brief Dequeue a data object from the queue at the given priority level. A blocking call to dequeue (blocking = true, default) will wait for an 
     * enqueue event if the queue of the given priority level is currently empty. A non blocking call will immediately access the first element of the given
     * priority level.
     * (!) Note: A non blocking call to an empty queue will lead to undefined behavior. Thus the empty() method needs to be called first.
     * (!) Note: A clean implementation would use exceptions to avoid a call to an empty queue.
     * 
     * @param priority_level 
     * @param blocking 
     * @return T 
     */
    T dequeue(int priority_level, bool blocking = true)
    {
        std::unique_lock<std::mutex> lock(queue_lock);
        if(blocking)
        {
            while(empty(priority_level))
            {
                // release lock as long as the wait and reaquire it afterwards.
                enqueue_event.wait(lock);
            } 
        }
        T val = priority_queue[priority_level].front();
        T copy = T(val);
        priority_queue[priority_level].pop();
        return copy; 
    }

    /**
     * @brief Dequeue the data object with the highest priority from the queue. A blocking call to dequeue (blocking = true, default) will wait for an 
     * enqueue event if the queue of the given priority level is currently empty. A non blocking call will immediately access the first element of the highest
     * non empty priority queue.
     * (!) Note: A non blocking call to an empty queue will lead to a undefined return value. Thus the empty() method needs to be called first.
     * (!) Note: A clean implementation would use exceptions to avoid a call to an empty queue.
     * 
     * @param blocking 
     * @return T 
     */
    T prio_dequeue(bool blocking = true)
    {
        int prio_index = 0;
        std::unique_lock<std::mutex> lock(queue_lock);
        if(blocking)
        {
            while(empty())
            {
                // release lock as long as the wait and reaquire it afterwards.
                enqueue_event.wait(lock);
            }
        }
        prio_index = prio_empty();
        T val;
        if(prio_index < priority_levels)
        {            
            val = priority_queue[prio_index].front();            
            priority_queue[prio_index].pop(); 
        }

        return val;
    }

    /**
     * @brief Faster priority dequeue. Use prio_empty first to determine the index for dequeuing.
     * 
     * @param prio_index the priority level for dequeuing
     * @return T queue entry
     */
    T fast_prio_dequeue(int prio_index)
    {
        std::unique_lock<std::mutex> lock(queue_lock);
        T val = priority_queue[prio_index].front();
        priority_queue[prio_index].pop(); 
        return val;
    }

    /**
     * @brief Get the number of priority levels
     * 
     * @return int 
     */
    int get_priority_levels()
    {
        return priority_levels;
    }

    /**
     * @brief Thread safe wrapper for std::queue swap. Swaps the content with the given queue.
     * 
     * @param queue The queue used for the swap operation.
     * @param priority_level The priority level for which the swap shall be executed.
     */
    void swap(std::queue<T> &queue ,int priority_level)
    {
        std::lock_guard<std::mutex> lock(queue_lock);
        priority_queue[priority_level].swap(queue);
        enqueue_event.notify_one();
    }

    /**
     * @brief Prints the size of the priority level queues.
     * 
     */
    void print()
    {
        for(int i=0; i<priority_levels; i++)
        {
            RM_logInfo("Queue " << i << std::endl)
            if(priority_queue[i].empty())
            {
                RM_logInfo("Empty" << std::endl)
            }
            else
            {
                RM_logInfo(" Size: " << priority_queue[i].size() << std::endl)
            }
        }
    }
    private:
    
    /**
     * @brief Vector containing a queue for every priority level
     * 
     */
    std::vector<std::queue<T>> priority_queue;
    
    /**
     * @brief Mutex for thread safe access to the priority queues
     * 
     */
    mutable std::mutex queue_lock;
    
    /**
     * @brief condition variable to signal an enqueue event. Used for any blocking dequeue() call.
     * 
     */
    std::condition_variable enqueue_event;
    
    /**
     * @brief The number of prioty levels used.
     * 
     */
    const int priority_levels;

};

#endif