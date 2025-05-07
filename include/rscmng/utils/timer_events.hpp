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


#ifndef TIMER_EVENTS_h
#define TIMER_EVENTS_h


#include <thread>
#include <iostream>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <rscmng/utils/log.hpp>
#include <rscmng/attributes/uuid.hpp>


namespace rscmng{

template<class T>
class TimerManager
{
    public:
        
    TimerManager();
    
    ~TimerManager();

    void start();

    void stop();
    
    void cancel(UUID_t id, bool repeat);

    template <typename DurationType, typename FunctionType, typename... Args>
    void registerTimer(UUID_t id, const DurationType& duration, bool repeat, FunctionType&& function, Args&&... args)
    {
        //RM_logInfo("Timer registered");
        auto entry = timer_repeat.find(id);
        if(entry == timer_repeat.end())
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            timer_repeat.insert({id, repeat});
            lock.unlock();
        }
        
        // Create a timer and bind it to the function
        auto timer = std::make_shared<boost::asio::basic_waitable_timer<T>>(timer_io, duration);
        timer->async_wait([timer, function, id, duration, this, args...](const boost::system::error_code& ec) {
            if (!ec) 
            {
                //RM_logInfo("Timer triggered")

                auto task = std::bind(function, args...);
                task();                
                // function()
                auto entry = timer_repeat.find(id);
                if(entry->second == true)
                {
                    //RM_logInfo("Re-register timer: " << id)
                    registerTimer(id, duration, true, function, args...);
                }
            }
            if(ec)
            {
                //RM_logInfo("Cancel called: " << id)
            }
        });
        // Add the timer to the list of timers
        std::unique_lock<std::mutex> lock(m_mutex);
        m_timers.erase(id);
        m_timers.emplace(id, std::move(timer));
    };

    private:
    boost::asio::io_service timer_io;
    boost::asio::io_service::work io_work;
    
    std::thread executor;
    
    std::map<UUID_t, bool> timer_repeat;
    std::map<UUID_t, std::shared_ptr<boost::asio::basic_waitable_timer<T>>> m_timers;
    std::mutex m_mutex;
};

#include <rscmng/utils/timer_events.tpp>

}; // End rscmng namespace

#endif