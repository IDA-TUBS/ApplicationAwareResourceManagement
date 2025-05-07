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


#ifndef SAFE_MAP_h
#define SAFE_MAP_h

#include <map>
#include <mutex>

/**
 * @brief A thread safe map
 * 
 * @tparam D type of the key
 * @tparam T type of the value
 */
template <class D, class T>
class SharedMap
{
    public:

    /**
     * @brief Thread safe wrapper for std::map insert()
     * 
     * @param value The key, value pair to be inserted in the map
     */
    void insert(std::pair<D, T> value)
    {
        std::lock_guard<std::mutex> lock(map_lock);
        shared_map.insert(value);
    }

    /**
     * @brief Thread safe wrapper for std::map find()
     * 
     * @param key The key to find
     * @return std::map<D, T>::iterator 
     */
    typename std::map<D, T>::iterator find(D key)
    {
        std::lock_guard<std::mutex> lock(map_lock);
        return shared_map.find(key);
    }

    /**
     * @brief Thread safe wrapper for std::map begin()
     * 
     * @return std::map<D, T>::iterator 
     */
    typename std::map<D, T>::iterator begin()
    {
        std::lock_guard<std::mutex> lock(map_lock);
        return shared_map.begin();
    }

    /**
     * @brief Thread safe wrapper for std::map end()
     * 
     * @return std::map<D, T>::iterator 
     */
    typename std::map<D, T>::iterator end()
    {
        std::lock_guard<std::mutex> lock(map_lock);
        return shared_map.end();
    }

    /**
     * @brief Thread safe wrapper for the std::map [] operator
     * 
     * @param key The key to be inserted/accessed
     * @return T& The value corresponding to the specified key
     */
    T& operator[](D key)
    {
        std::lock_guard<std::mutex> lock(map_lock);
        return shared_map[key];
    }

    /**
     * @brief Thread safe wrapper for std::map erase()
     * 
     * @param key The key of the entry which should be erased
     * @return size_type number of elements removed (0 or 1)
     */
    size_t erase(D key)
    {
        std::lock_guard<std::mutex> lock(map_lock);
        return shared_map.erase(key);
    }

    /**
     * @brief Wrapper for sd::map size()
     * 
     * @return size_t 
     */
    size_t size()
    {
        return shared_map.size();
    }

    /**
     * @brief Construct a new (empty) Shared Map object
     * 
     */
    SharedMap(void)
    :   shared_map(),
        map_lock()
    {}

    /**
     * @brief Destroy the Shared Map object (default)
     * 
     */
    ~SharedMap(void)
    {}

    protected:

    /**
     * @brief The underlying map for implementing a thread safe map
     * 
     */
    std::map<D, T> shared_map;

    /**
     * @brief The mutex for concurrent access of the std::map
     * 
     */
    mutable std::mutex map_lock;
};

#endif