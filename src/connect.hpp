#pragma once 
#include <iostream>

#define REGISTER 0 //用户注册标识
#define LOGIN 1 //用户登录标识

//用户注册信息
struct RegisterInfo
{
    char nickname_[20];
    char school_[20];
    char passwd_[10]; //注册密码
};

//用户登录信息
struct LoginInfo
{
    uint32_t userid; //返回给用户的ID号
    char passwd_[10]; //登录密码
};

//服务端回应信息
struct ReplyInfo
{
    int status; //当前用户状态：登录成功 || 注册成功 || 登录失败 || 登陆成功
    uint32_t userid;
};

//服务端处理客户端请求后的结果状态
enum UserStatus
{
    REGISTERSUCCESS = 0, //注册成功
    REGISTERFAILED, //注册失败
    LOGINSUCCESS,        //登录成功
    LOGINFAILED    //登录失败
};

//注册登录连接
class RegLogConnect
{
    public:
        RegLogConnect(int sock, void* chatsvr)
        {
            sock_ = sock;
            chatsvr_ = chatsvr;
        }

        ~RegLogConnect()
        {}

        int GetSock()
        {
            return sock_;
        }

        void* GetChatSvr()
        {
            return chatsvr_;
        }
    private:
        int sock_;
        //用来保存ChatSvr类的实例化指针
        void* chatsvr_;
};
