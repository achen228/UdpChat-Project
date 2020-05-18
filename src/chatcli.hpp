#pragma once 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <vector>
#include <json/json.h>

#include "log.hpp"
#include "connect.hpp"

#define UDP_PORT 17777
#define TCP_PORT 17778

struct MySelf
{
    std::string nickname_;
    std::string school_;
    std::string passwd_;
    uint32_t userid_;
};

class ChatCli
{
    public:
        ChatCli(std::string svrip = "127.0.0.1")
        {
            udp_sock_ = -1;
            udp_port_ = UDP_PORT;
            tcp_sock_ = -1;
            tcp_port_ = TCP_PORT;
            svrip_ = svrip;
        }

        ~ChatCli()
        {
            close(udp_sock_);
            udp_sock_ = -1;
        }

        void Init()
        {
            udp_sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(udp_sock_ < 0)
            {
                LOG(FATAL, "Create udp socket failed") << std::endl;
                exit(1);
            }
        }

        bool ConnectSvr()
        {
            tcp_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(tcp_sock_ < 0)
            {
                LOG(FATAL, "Create tcp socket failed") << std::endl;
                exit(2);
            }

            struct sockaddr_in svraddr;
            svraddr.sin_family = AF_INET;
            svraddr.sin_port = htons(tcp_port_);
            svraddr.sin_addr.s_addr = inet_addr(svrip_.c_str());
            int ret = connect(tcp_sock_, (struct sockaddr*)&svraddr, sizeof(svraddr));
            if(ret < 0)
            {
                LOG(ERROR, "Connect server failed") << std::endl;
                return false;
            }
            return true;
        }

        bool Register()
        {
            if(!ConnectSvr())
            {
                return false;
            }

            //1.发送注册标识
            char logo = REGISTER;
            ssize_t send_size = send(tcp_sock_, &logo, 1, 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Send register logo failed") << std::endl;
                return false;
            }

            //2.发送注册内容
            RegisterInfo ri;
            std::cout << "Please enter nickname:";
            std::cin >> ri.nickname_;
            std::cout << "Please enter school:";
            std::cin >> ri.school_;
            while(1)
            {
                std::string passwd_one;
                std::string passwd_two;
                std::cout << "Please enter passwd:";
                std::cin >> passwd_one;
                std::cout << "Please enter passwd again:";
                std::cin >> passwd_two;
                if(passwd_one == passwd_two)
                {
                    strcpy(ri.passwd_, passwd_one.c_str());
                    break;
                }
                else 
                {
                    LOG(ERROR, "Two passwords do not match") << std::endl;
                }
            }
            send_size = send(tcp_sock_, &ri, sizeof(ri), 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Send register info failed") << std::endl;
                return false;
            }
            //3.解析应答状态和获取userid
            ReplyInfo rpi;
            int recv_size = recv(tcp_sock_, &rpi, sizeof(rpi), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv server reply failed") << std::endl;
                return false;
            }
            else if(recv_size == 0)
            {
                LOG(ERROR, "Server shutdown connect") << std::endl;
                return false;
            }

            if(rpi.status == REGISTERSUCCESS)
            {
                LOG(INFO, "User register success, userid is ") << rpi.userid << std::endl;
            }
            else 
            {
                LOG(ERROR, "User register failed") << std::endl;
                return false;
            }

            //4.保存服务端返回的userid
            me_.nickname_ = ri.nickname_;
            me_.school_ = ri.school_;
            me_.passwd_ = ri.passwd_;
            me_.userid_ = rpi.userid;

            close(tcp_sock_);
            return true;
        }

        bool Login()
        {
            if(!ConnectSvr())
            {
                return false;
            }

            //1.发送登录标识
            char logo = LOGIN;
            int send_size = send(tcp_sock_, &logo, 1, 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Send login logo failed") << std::endl;
                return false;
            }

            //2.发送登录内容
            LoginInfo li;
            std::cout << "Please enter userid:";
            std::cin >> li.userid;
            std::cout << "Please enter passwd:";
            std::cin >> li.passwd_;
            send_size = send(tcp_sock_, &li, sizeof(li), 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Send login info failed") << std::endl;
                return false;
            }

            //3.解析应答状态
            ReplyInfo ri;
            ssize_t recv_size = recv(tcp_sock_, &ri, sizeof(ri), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv server reply failed") << std::endl;
                return false;
            }
            else if(recv_size == 0)
            {
                LOG(ERROR, "Server shutdown connect") << std::endl;
                return false;
            }

            if(ri.status == LOGINSUCCESS)
            {
                LOG(INFO, "User login success, Please begin chat") << std::endl;
            }
            else 
            {
                LOG(ERROR, "User login failed") << std::endl;
                return false;
            }
            
            close(tcp_sock_);
            return true;
        }

        bool SendMsg(std::string& msg)
        {
            struct sockaddr_in svraddr;
            svraddr.sin_family = AF_INET;
            svraddr.sin_port = htons(udp_port_);
            svraddr.sin_addr.s_addr = inet_addr(svrip_.c_str());
            ssize_t send_size = sendto(udp_sock_, msg.c_str(), msg.size(), 0, (struct sockaddr*)&svraddr, sizeof(svraddr));
            if(send_size < 0)
            {
                LOG(ERROR, "Send msg failed") << std::endl;
                return false;
            }
            return true;
        }

        bool RecvMsg(std::string& msg)
        {
            char buf[1024] = {0};
            struct sockaddr_in svraddr;
            socklen_t svraddr_len;
            ssize_t recv_size = recvfrom( udp_sock_, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&svraddr, &svraddr_len);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv msg failed") << std::endl;
                return false;
            }
            msg.assign(buf, recv_size);
            return true;
        }

        MySelf& GetMySelf()
        {
            return me_;
        }

        void PushUser(std::string& userinfo)
        {
            auto iter = vec_.begin();
            for(; iter != vec_.end(); iter++)
            {
                if(*iter == userinfo)
                {
                    return;
                }
            }
            vec_.push_back(userinfo);
            return;
        }

        std::vector<std::string>& GetOnlineUserList()
        {
            return vec_;
        }

    private:
        int udp_sock_;
        int udp_port_;
        int tcp_sock_;
        int tcp_port_;
        std::string svrip_;
        MySelf me_;
        //保存在线的用户
        std::vector<std::string> vec_;
};
