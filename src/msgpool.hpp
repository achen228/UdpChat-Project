#pragma once
#include <pthread.h>
#include <iostream>
#include <queue>
#include <string>

#define Max_CAPACITY_QUE 1024

class MsgPool
{
    public:
        MsgPool()
        {
            capacity_ = Max_CAPACITY_QUE;
            pthread_mutex_init(&lock_que_, NULL);
            pthread_cond_init(&pro_que_, NULL);
            pthread_cond_init(&con_que_, NULL);
        }

        ~MsgPool()
        {
            pthread_mutex_destroy(&lock_que_);
            pthread_cond_destroy(&pro_que_);
            pthread_cond_destroy(&con_que_);

        }

        void PushQue(std::string& msg)
        {
            pthread_mutex_lock(&lock_que_);
            while(IsFull())
            {
                pthread_cond_wait(&pro_que_, &lock_que_);
            }
            msg_que_.push(msg);
            pthread_mutex_unlock(&lock_que_);
            pthread_cond_signal(&con_que_);
        }

        void PopQue(std::string& msg)
        {
            pthread_mutex_lock(&lock_que_);
            while(msg_que_.empty())
            {
                pthread_cond_wait(&con_que_, &lock_que_);
            }
            msg = msg_que_.front();
            msg_que_.pop();
            pthread_mutex_unlock(&lock_que_);
            pthread_cond_signal(&pro_que_);
        }
    private:
        bool IsFull()
        {
            if(msg_que_.size() == capacity_)
            {
                return true;
            }
            return false;
        }
    private:
        //约束队列大小，防止队列无限扩容
        size_t capacity_;
        //消息队列
        std::queue<std::string> msg_que_;
        //互斥锁
        pthread_mutex_t lock_que_;
        //生产者条件变量
        pthread_cond_t pro_que_;
        //消费者条件变量
        pthread_cond_t con_que_;
};
