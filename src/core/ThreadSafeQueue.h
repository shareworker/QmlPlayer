#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <QMutex>
#include <QWaitCondition>
#include <cstddef>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t capacity = 100)
        : capacity_(capacity), stopped_(false) {}
    ~ThreadSafeQueue() { stop(); }

    void push(T value) {
        QMutexLocker locker(&mutex_);
        while (size_ >= capacity_ && !stopped_) {
            notFull_.wait(&mutex_);
        }
        if (stopped_) {
            return;
        }
        queue_.push(value);
        size_++;
        notEmpty_.wakeOne();
    }

    bool pop(T& value) {
        QMutexLocker locker(&mutex_);
        while(size_ == 0 && !stopped_) {
            notEmpty_.wait(&mutex_);
        }

        if (stopped_ || size_ == 0) return false;
        value = std::move(queue_.front());
        queue_.pop();
        size_--;
        notFull_.wakeOne();
        return true;
    }

    bool tryPop(T& value) {
        QMutexLocker locker(&mutex_);
        if (size_ == 0) return false;
        value = std::move(queue_.front());
        queue_.pop();
        size_--;
        notFull_.wakeOne();
        return true;
    }

    void clear() {
        QMutexLocker locker(&mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        size_ = 0;
        notFull_.wakeAll();
    }

    void stop() {
        QMutexLocker locker(&mutex_);
        stopped_ = true;
        notEmpty_.wakeAll();
        notFull_.wakeAll();
    }

    void start() {
        QMutexLocker locker(&mutex_);
        stopped_ = false;
    }

    size_t size() const {
        QMutexLocker locker(&mutex_);
        return size_;
    }

    bool empty() const {
        QMutexLocker locker(&mutex_);
        return size_ == 0;
    }

    size_t capacity() const {
        return capacity_;
    }

private:
    size_t capacity_;
    bool stopped_;
    mutable QMutex mutex_;
    QWaitCondition notEmpty_;
    QWaitCondition notFull_;
    std::queue<T> queue_;
    size_t size_ = 0;
};

#endif // THREADSAFEQUEUE_H