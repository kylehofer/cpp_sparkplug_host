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
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>

/**
 * @brief An owning collection of objects that are looked up by a unique name.
 *
 * Items are created on demand by get() and destroyed by clear() or when the collection is destroyed.
 * The collection is not copyable, since it owns its items.
 *
 * @tparam T The item type. Must be constructible from a const std::string &.
 */
template <class T>
class DataCollection
{
private:
    std::map<std::string, std::unique_ptr<T>> items;

protected:
    /**
     * @brief Perform an action on each item in the collection
     *
     * @param callback
     */
    void each(const std::function<void(T *)> &callback)
    {
        for (auto &item : items)
        {
            callback(item.second.get());
        }
    }

    /**
     * @brief Check if any of the items in the collection match a condition
     *
     * @param callback
     * @return true
     * @return false
     */
    bool any(const std::function<bool(T *)> &callback)
    {
        return std::any_of(
            items.begin(),
            items.end(),
            [&callback](const auto &item)
            { return callback(item.second.get()); });
    }

    /**
     * @brief Get the number of items in the collection
     *
     * @return size_t
     */
    size_t size() const
    {
        return items.size();
    }

public:
    DataCollection() = default;
    DataCollection(const DataCollection &) = delete;
    DataCollection &operator=(const DataCollection &) = delete;

    /**
     * @brief Gets the item in the collection that matches the name.
     * If no item exists a new one will be constructed using the name.
     *
     * @param name
     * @return T* Never null. Owned by the collection.
     */
    T *get(const std::string &name)
    {
        auto &item = items[name];
        if (!item)
        {
            item = std::make_unique<T>(name);
        }
        return item.get();
    }

    /**
     * @brief Gets the item in the collection that matches the name, without creating it.
     *
     * @param name
     * @return T* The item, or nullptr if it does not exist. Owned by the collection.
     */
    T *find(const std::string &name)
    {
        auto item = items.find(name);
        return item == items.end() ? nullptr : item->second.get();
    }

    /**
     * @brief Empties the collection, destroying every item
     *
     */
    void clear()
    {
        items.clear();
    }

    virtual ~DataCollection() = default;
};

#endif /* SRC_DATACOLLECTION */
