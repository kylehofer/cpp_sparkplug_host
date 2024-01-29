/*
 * File: DataCollection.h
 * Project: cpp_sparkplug_host
 * Created Date: Sunday December 24th 2023
 * Author: Kyle Hofer
 *
 * MIT License
 *
 * Copyright (c) 2024 Kyle Hofer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * HISTORY:
 */

#ifndef SRC_DATACOLLECTION
#define SRC_DATACOLLECTION

#include "types/CommonTypes.h"
#include <string>
#include <map>
#include <functional>

/**
 * @brief Adds a collection of objects to a class that can be accessed by unique names
 *
 * @tparam T
 */
template <class T>
class DataCollection
{
private:
    std::map<std::size_t, T *> items;

protected:
    /**
     * @brief Perform an action on each item in the collection
     *
     * @param callback
     */
    void each(std::function<void(T *)> callback)
    {
        for_each(
            items.begin(),
            items.end(),
            [callback](std::pair<const long unsigned int, T *> item)
            {
                callback(item.second);
            });
    };

    /**
     * @brief Check if any of the items in the collection match a condition
     *
     * @param callback
     * @return true
     * @return false
     */
    bool any(std::function<bool(T *)> callback)
    {
        return any_of(
            items.begin(),
            items.end(),
            [callback](std::pair<const long unsigned int, T *> item)
            { return callback(item.second); });
    }

    /**
     * @brief Get the number of items in the collection
     *
     * @return size_t
     */
    size_t size()
    {
        return items.size();
    }

public:
    /**
     * @brief Gets
     *
     * @param name
     * @return T*
     */
    T *get(std::string &name)
    {
        std::hash<std::string> hasher;
        auto key = hasher(name);
        if (!items.contains(key))
        {
            items[key] = new T(name);
        }

        return items[key];
    }

    /**
     * @brief Gets the item in the collection that matches the name
     * If no item exists a new one will be constructed using the name
     *
     * @param name
     * @return T*
     */
    T *get(const char *name)
    {
        std::string input(name);
        return get(input);
    }

    /**
     * @brief Empties the collection of items
     *
     */
    void clear()
    {
        each([](T *item)
             { delete item; });
        items.clear();
    }

    /**
     * @brief Destroy the Data Collection object
     *
     */
    virtual ~DataCollection()
    {
        clear();
    }
};

#endif /* SRC_DATACOLLECTION */
