#pragma once 
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

//保存当前用户的状态
#define ONLINE 0    //在线
#define OFFLINE 1   //离线
#define REGISTERED 2//已注册
#define LOGINED 3   //已登录

class UserInfo
{
    public:
        //在注册和登录阶段使用的是TCP，所以不能保存TCP的地址信息
        //而是要等到当前登录上来的这个用户第一次使用UDP协议发送消息的时候，将UDP的地址信息保存下来
        //保存下来之后，进行群发的时候，就可以找到有效的UDP地址信息
        UserInfo(const std::string& nickname, const std::string& school, const std::string& passwd, uint32_t userid)
        {
            nickname_ = nickname;
            school_ = school;
            passwd_ = passwd;
            userid_ = userid;
            memset(&cliaddr_, '0', sizeof(struct sockaddr_in));
            cliaddr_len_ = -1;
            userstatus_ = OFFLINE;
        }

        ~UserInfo()
        {}

        void SetUserStatus(const int userstatus)
        {
            userstatus_ = userstatus;
        }

        void SetCliAddr(const struct sockaddr_in& cliaddr)
        {
            memcpy(&cliaddr_, &cliaddr, sizeof(cliaddr));
        }

        void SetCliAddrLen(const socklen_t& cliaddr_len)
        {
            cliaddr_len_ = cliaddr_len;
        }

        std::string& GetPasswd()
        {
            return passwd_;
        }

        int& GetUserStatus()
        {
            return userstatus_;
        }

        struct sockaddr_in& GetOnlineCliAddr()
        {
            return cliaddr_;
        }

        socklen_t& GetOnlineCliAddrLen()
        {
            return cliaddr_len_;
        }
    private:
        std::string nickname_;
        std::string school_;
        std::string passwd_;
        uint32_t userid_;
        //保存UDP客户端的地址信息
        struct sockaddr_in cliaddr_;
        socklen_t cliaddr_len_;
        //保存当前用户的状态
        int userstatus_;
};

class UserManager
{
    public:
        UserManager()
        {
            map_.clear();
            vec_.clear();
            prepare_userid_ = 0;
            pthread_mutex_init(&lock_, NULL);
        }

        ~UserManager()
        {
            pthread_mutex_destroy(&lock_);
        }

        //服务端获取在线用户信息
        void GetOnlineUserInfo(std::vector<UserInfo>* vec)
        {
            *vec = vec_;
        }

        //管理模块处理用户注册的信息:需要用户提供注册的 nickname,school,passwd, 且返回给用户一个userid
        int Register(const std::string& nickname, const std::string& school, const std::string& passwd, uint32_t* userid)
        {
            if(nickname.size() == 0 || school.size() == 0 || passwd.size() == 0)
            {
                return -1;
            }

            pthread_mutex_lock(&lock_);
            UserInfo userinfo(nickname, school, passwd, prepare_userid_);
            //更改当前用户的状态为 已注册
            userinfo.SetUserStatus(REGISTERED);
            //将注册用户插入到map中
            map_.insert(std::make_pair(prepare_userid_, userinfo));
            //将ID返回给上层
            *userid = prepare_userid_;
            prepare_userid_++;
            pthread_mutex_unlock(&lock_);

            return 0;
        }

        //管理模块处理用户登录的信息
        int Login(const uint32_t& userid, const std::string& passwd)
        {
            if(passwd.size() < 0)
            {
                return -1;
            }

            int login_status;
            
            //返回用户登录的状态
            //先在map中查找是否存在这样的ID
            //不存在
            //存在：密码正确 || 密码错误
            std::unordered_map<uint32_t, UserInfo>::iterator iter;
            pthread_mutex_lock(&lock_); //防止迭代器失效
            iter = map_.find(userid);
            if(iter != map_.end())
            {
                //查找到了
                if(passwd == iter->second.GetPasswd())
                {
                    //密码正确
                    //更改当前用户的状态 已登录
                    iter->second.GetUserStatus() = LOGINED;
                    login_status = 0;
                }
                else 
                {
                    //密码不正确
                    LOG(ERROR, "Passwd is false") << std::endl;
                    login_status = -1;
                }
            }
            else 
            {
                //未查找到
                LOG(ERROR, "UserId is false") << std::endl;
                login_status = -1;
            }
            pthread_mutex_unlock(&lock_);
            return login_status;
        }

        bool IsLogin(uint32_t userid, const struct sockaddr_in& cliaddr, const socklen_t& cliaddr_len)
        {
            if(sizeof(cliaddr) < 0 || cliaddr_len < 0)
            {
                return false;
            }

            //检测当前用户是否存在
            std::unordered_map<uint32_t, UserInfo>::iterator iter;
            pthread_mutex_lock(&lock_); //防止迭代器失效
            iter = map_.find(userid);
            if(iter == map_.end())
            {
                pthread_mutex_unlock(&lock_);
                LOG(ERROR, "User not exist") << std::endl;
                return false;
            }

            //如果存在，判断当前用户的状态
            if(iter->second.GetUserStatus() == OFFLINE || iter->second.GetUserStatus() == REGISTERED)
            {
                pthread_mutex_unlock(&lock_);
                LOG(ERROR, "User status error") << std::endl;
                return false;
            }

            //判断当前用户是否是第一次发送消息
            if(iter->second.GetUserStatus() == ONLINE)
            {
                pthread_mutex_unlock(&lock_);
                //不是第一次发送消息，属于已在线用户
                return true;
            }

            //如果是第一次发送消息，属于新在线用户
            if(iter->second.GetUserStatus() == LOGINED)
            {
                //添加该用户的地址信息
                iter->second.SetCliAddr(cliaddr);
                iter->second.SetCliAddrLen(cliaddr_len);
                //更改当前用户的状态为 ONLINE 
                iter->second.SetUserStatus(ONLINE);
                //将新在线用户的信息保存到 vector中
                vec_.push_back(iter->second);
            }

            pthread_mutex_unlock(&lock_);
            return true;
        }
    private:
        pthread_mutex_t lock_;
        //保存所有注册用户的信息--TCP
        std::unordered_map<uint32_t, UserInfo> map_;
        //保存所有在线用户的信息--UDP，判断是否在线的标准，判断是否使用UDP发送消息
        std::vector<UserInfo> vec_;
        //预分配的用户ID
        uint32_t prepare_userid_;
};
