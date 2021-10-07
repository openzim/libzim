/*
 * Copyright (C) 2016-2020 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef OPENZIM_LIBZIM_QUEUE_H
#define OPENZIM_LIBZIM_QUEUE_H

#define MAX_QUEUE_SIZE 10

#include <mutex>
#include <queue>
#include "../tools.h"

template<typename T>
class Queue {
    public:
        Queue() = default;
        virtual ~Queue() = default;
        virtual bool isEmpty();
        virtual size_t size();
        virtual void pushToQueue(const T& element);
        virtual bool getHead(T &element);
        virtual bool popFromQueue(T &element);

    protected:
        std::queue<T>   m_realQueue;
        std::mutex      m_queueMutex;

    private:
        // Make this queue non copyable
        Queue(const Queue&);
        Queue& operator=(const Queue&);
};

template<typename T>
bool Queue<T>::isEmpty() {
    std::lock_guard<std::mutex> l(m_queueMutex);
    return m_realQueue.empty();
}

template<typename T>
size_t Queue<T>::size() {
    std::lock_guard<std::mutex> l(m_queueMutex);
    return m_realQueue.size();
}

template<typename T>
void Queue<T>::pushToQueue(const T &element) {
    unsigned int wait = 0;
    unsigned int queueSize = 0;

    do {
        zim::microsleep(wait);
        queueSize = size();
        wait += 10;
    } while (queueSize > MAX_QUEUE_SIZE);

    std::lock_guard<std::mutex> l(m_queueMutex);
    m_realQueue.push(element);
}

template<typename T>
bool Queue<T>::getHead(T &element) {
    std::lock_guard<std::mutex> l(m_queueMutex);
    if (m_realQueue.empty()) {
        return false;
    }
    element = m_realQueue.front();
    return true;
}

template<typename T>
bool Queue<T>::popFromQueue(T &element) {
    std::lock_guard<std::mutex> l(m_queueMutex);
    if (m_realQueue.empty()) {
        return false;
    }

    element = m_realQueue.front();
    m_realQueue.pop();

  return true;
}

#endif // OPENZIM_LIBZIM_QUEUE_H
