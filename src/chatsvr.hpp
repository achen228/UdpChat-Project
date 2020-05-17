#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <string>

#include "msgpool.hpp"
#include "log.hpp"
#include "connect.hpp"
#include "usermanager.hpp"
#include "jsonmessage.hpp"

#define UDP_PORT 17777
#define TCP_PORT 17778
#define THREAD_COUNT 2

class ChatSvr
{
    public:
        ChatSvr()
        {
            udpsock_ = -1;
            udpport_ = UDP_PORT; 
            msgpool_ = NULL;
            tcpsock_ = -1;
            tcpport_ = TCP_PORT;
            usermana_ = NULL;
        }

        ~ChatSvr()
        {
            if(msgpool_)
            {
                delete msgpool_;
                msgpool_ = NULL;
            }

            if(usermana_)
            {
                delete usermana_;
                usermana_ = NULL;
            }
        }

        void Init()
        {
            //创建UDP套接字
            udpsock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(udpsock_ < 0)
            {
                LOG(FATAL, "Create udp socket failed") << std::endl;
                exit(1);
            }
            LOG(INFO, "Create udp socket success") << std::endl;

            //绑定UDP地址信息
            struct sockaddr_in addr_udp;
            addr_udp.sin_family = AF_INET;
            addr_udp.sin_port = htons(udpport_);
            addr_udp.sin_addr.s_addr = inet_addr("0.0.0.0");
            int ret = bind(udpsock_, (struct sockaddr*)&addr_udp, sizeof(addr_udp));
            if(ret < 0)
            {
                LOG(FATAL, "Bind udp addrinfo failed") << std::endl;
                exit(2);
            }
            LOG(INFO, "Bind udp addrinfo success ") << "0.0.0.0:17777" << std::endl;

            //初始化数据池
            msgpool_ = new MsgPool();
            if(!msgpool_)
            {
                LOG(FATAL, "Create msgpool failed") << std::endl;
                exit(3);
            }
            LOG(INFO, "Create msgpool success") << std::endl;

            //创建TCP套接字
            tcpsock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(tcpsock_ < 0)
            {
                LOG(FATAL, "Create tcp socket failed") << std::endl;
                exit(4);
            }
            LOG(INFO, "Create tcp socket success") << std::endl;

            //地址复用
            int opt = 1;
            setsockopt(tcpsock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            //绑定TCP地址信息
            struct sockaddr_in addr_tcp;
            addr_tcp.sin_family = AF_INET;
            addr_tcp.sin_port = htons(tcpport_);
            addr_tcp.sin_addr.s_addr = inet_addr("0.0.0.0");
            ret = bind(tcpsock_, (struct sockaddr*)&addr_tcp, sizeof(addr_tcp));
            if(ret < 0)
            {
                LOG(FATAL, "Bind tcp addrinfo failed") << std::endl;
                exit(5);
            }
            LOG(INFO, "Bind tcp addrinfo success ") << "0.0.0.0:17778" << std::endl;

            //TCP监听
            ret = listen(tcpsock_, 5);
            if(ret < 0)
            {
                LOG(FATAL, "Tcp listen failed") << std::endl;
                exit(6);
            }
            LOG(INFO, "Tcp listen success") << std::endl;

            //初始化用户管理模块
            usermana_ = new UserManager();
            if(!usermana_)
            {
                LOG(FATAL, "Create user manager failed") << std::endl;
                exit(7);
            }
            LOG(INFO, "Create user manager success") << std::endl;
        }

        //生产线程和消费线程开始工作
        void Start()
        {
            pthread_t tid;
            for(int i = 0; i < THREAD_COUNT; i++)
            {
                int ret = pthread_create(&tid, NULL, ProductMsgStart, (void*)this);
                if(ret < 0)
                {
                    LOG(ERROR, "Create product thread failed") << std::endl;
                    exit(8);
                }

                ret = pthread_create(&tid, NULL, ConsumeMsgStart, (void*)this);
                if(ret < 0)
                {
                    LOG(ERROR, "Create product thread failed") << std::endl;
                    exit(9);
                }
            }
            LOG(INFO, "UdpChat server start success") << std::endl;

            while(1)
            {
                //TCP接收
                struct sockaddr_in cliaddr;
                socklen_t cliaddr_len = sizeof(cliaddr);
                int newsock = accept(tcpsock_, (struct sockaddr*)&cliaddr, &cliaddr_len);
                if(newsock < 0)
                {
                    LOG(ERROR, "Accept new connect failed") << std::endl;
                    continue;
                }
                LOG(INFO, "Accept new connect success") << std::endl;

                //创建线程处理用户注册、登录请求
                RegLogConnect* co = new RegLogConnect(newsock, (void*)this); 
                if(!co)
                {
                    LOG(ERROR, "Create reglog connect failed") << std::endl;
                    continue;
                }
                pthread_t tid;
                int ret = pthread_create(&tid, NULL, RegLogStart, (void*)co);
                if(ret < 0)
                {
                    LOG(ERROR, "Create reglog thread failed") << std::endl;
                    continue;
                }
                LOG(INFO, "Create tcp reglog connect success") << std::endl;
            }
        }
    private:
        static void* ProductMsgStart(void* arg)
        {
            pthread_detach(pthread_self());
            ChatSvr* cs = (ChatSvr*)arg;
            while(1) //while(1)循环，生产线程将一直在，主线程main退出时它才会退出
            {
                cs->RecvMsg();
            }
            return NULL;
        }

        static void* ConsumeMsgStart(void* arg)
        {
            pthread_detach(pthread_self());
            ChatSvr* cs = (ChatSvr*)arg;
            while(1) //while(1)循环，消费线程将一直在，主线程main退出时它才会退出
            {
                cs->Broadcast();
            }
            return NULL;
        }

        static void* RegLogStart(void* arg)
        {
            pthread_detach(pthread_self());
            RegLogConnect* rlc = (RegLogConnect*)arg;

            //1.客户端发起连接请求
            char logo; //先接收一个标识，判断用户的请求类型
            ssize_t recv_size = recv(rlc->GetSock(), &logo, 1, 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv user request logo failed") << std::endl;
                return NULL;
            }
            else if(recv_size == 0)
            {
                LOG(ERROR, "Client shutdown connect") << std::endl;
                return NULL;
            }

            ChatSvr* cs = (ChatSvr*)rlc->GetChatSvr();
            int userstatus = -1; //保存服务端处理请求后用户的状态
            uint32_t userid = -1;     //服务端处理注册请求后返回给用户的ID

            switch(logo)
            {
                case REGISTER:
                    //服务端处理注册请求
                    userstatus = cs->DealRegister(rlc->GetSock(), &userid);
                    break;
                case LOGIN:
                    //服务端处理登录请求
                    userstatus = cs->DealLogin(rlc->GetSock());
                    break;
                default:
                    LOG(ERROR, "Recv user request logo not a effective value") << std::endl;
                    break;
            }
            
            // 2服务端对客户端的连接请求做出响应
            ReplyInfo ri;
            ri.status = userstatus;
            ri.userid = userid;
            ssize_t send_size = send(rlc->GetSock(), &ri, sizeof(ri), 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Server send replyinfo failed") << std::endl;
                //todo
                //如果发送失败，是否考虑应用层重新发送
            }
            LOG(INFO, "Server send replyinfo success") << std::endl;

            //释放TCP连接
            close(rlc->GetSock());
            delete rlc;
            return NULL;
        }
    private:
        int DealRegister(int sock, uint32_t* userid)
        {
            //1.接收用户注册信息
            RegisterInfo ri;
            ssize_t recv_size = recv(sock, &ri, sizeof(ri), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv user register info failed") << std::endl;
                return REGISTERFAILED;
            }
            else if(recv_size ==0)
            {
                LOG(ERROR, "Client shutdown connect") << std::endl;
                //todo
                //特殊处理对端关闭的情况
            }

            //2.调用用户管理模块进行注册请求的处理,返回注册成功之后给用户的userid
            int ret = usermana_->Register(ri.nickname_, ri.school_, ri.passwd_, userid); 
            if(ret == -1)
            {
                return REGISTERFAILED;
            }
            return REGISTERSUCCESS;
        }

        int DealLogin(int sock)
        {
            //1.接收用户登录信息
            LoginInfo li;
            ssize_t recv_size = recv(sock, &li, sizeof(li), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv user login info failed") << std::endl;
                return LOGINFAILED;
            }
            else if(recv_size == 0)
            {
                LOG(ERROR, "Client shutdown connect") << std::endl;
                //todo
                //特殊处理对端关闭的情况
            }

            //2.调用用户管理模块进行登录请求的处理
            int ret = usermana_->Login(li.userid, li.passwd_); 
            if(ret == -1)
            {
                return LOGINFAILED;
            }
            return LOGINSUCCESS;
        }

        void RecvMsg()
        {
            char buf[1024] = {0};
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(struct sockaddr_in);
            ssize_t recv_size = recvfrom(udpsock_, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&cliaddr, &cliaddr_len);
            if(recv_size < 0)
            {
                LOG(ERROR, "Recv msg failed") << std::endl;
            }
            else 
            {
                // 接收成功
                LOG(INFO, "Recv msg success ") << "[" << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << "]" 
                                               << buf << std::endl;        
                //正常逻辑
                std::string msg;
                msg.assign(buf, recv_size);
                //需要增加用户管理，只有已注册且登录的用户才可以向服务端发送消息
                //因为可能存在知道服务端地址和端口的用户，但是却没有注册或没有登录的用户发送消息
                //1.校验当前的消息是否属于新登录用户或者已登录用户发送的
                //  1.1不是，则认为是非法消息
                //  1.2是，区分一下是新登录用户发送消息还是已登录用户发送消息
                //      是新登录用户：保存UDP地址信息，并且更新状态为在线，将数据放到数据池中
                //      是已登录用户：直接将数据放到数据池中
                //2.校验的时候，需要和用户管理模块进行沟通, 并且需要知道发送消息用户的信息用来校验，但是UDP-recvfrom无法获取到用户的信息
                //3.需要将UDP-recvfrom接收到的数据从Json格式转化为我们可识别的数据
                //  客户端也需要将UDP-sendto发送的消息转换为Json格式
                JsonMessage jsonmsg;
                jsonmsg.Deserialization(msg);
                 
                bool ret = usermana_->IsLogin(jsonmsg.GetUserId(), cliaddr, cliaddr_len);
                if(ret != true)
                {
                    LOG(ERROR, "Server discard this msg, Illegal user! msg:") << msg << std::endl;
                    return ;
                }
                //将数据放到数据池当中去
                msgpool_->PushQue(msg);
                LOG(INFO, "Server push this msg to msgpool") << std::endl;
            }
        }

        //广播消息给所有在线的客户端
        void Broadcast()
        {
            //获取发送的内容
            std::string msg;
            msgpool_->PopQue(msg);
            LOG(INFO, "Server pop this msg from msgpool") << std::endl;
            //获取在线客户端的地址信息 SendMsg
            //用户管理模块提供在线的用户列表
            std::vector<UserInfo> vec;
            usermana_->GetOnlineUserInfo(&vec);
            std::vector<UserInfo>::iterator iter = vec.begin();
            for(; iter != vec.end(); iter++)
            {
                SendMsg(msg, iter->GetOnlineCliAddr(), iter->GetOnlineCliAddrLen());
            }
        }

        //给一个客户端发送消息
        void SendMsg(std::string& msg, struct sockaddr_in& cliaddr, socklen_t& cliaddr_len)
        {
            ssize_t send_size = sendto(udpsock_, msg.c_str(), msg.size(), 0, (struct sockaddr*)&cliaddr, cliaddr_len);
            if(send_size < 0)
            {
                LOG(ERROR, "Send msg failed") << std::endl;
                //todo
                //没有发送成功，是否需要重新缓存没有发送成功的信息和客户端的地址
            }
            else 
            {
                // 发送成功
                LOG(INFO, "Send msg success ") << "[" << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << "]"
                                               << msg << std::endl;        
            }
        }
    private:
        //UDP
        int udpsock_;
        int udpport_;
        //数据池
        MsgPool* msgpool_;
        //TCP
        int tcpsock_;
        int tcpport_;
        //用户管理
        UserManager* usermana_;
};
