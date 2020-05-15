#pragma once 
#include <iostream>
#include <string>
#include <json/json.h>

class JsonMessage
{
    public:
        //反序列化客户端发送给我们的json数据串
        void Deserialization(std::string& msg)
        {
            Json::Reader read;
            Json::Value val;
            read.parse(msg, val, false);

            nickname_ = val["nickname_"].asString();
            school_ = val["school_"].asString();
            msg_ = val["msg_"].asString();
            userid_ = val["userid_"].asInt();
        }

        //序列化接口
        void Serialize(std::string& msg)
        {
            Json::Value val;
            val["nickname_"] = nickname_;
            val["school_"] = school_;
            val["msg_"] = msg_;
            val["userid_"] = userid_;

            Json::FastWriter writer;
            msg = writer.write(val);
        }

        std::string& GetNickName()
        {
            return nickname_;
        }

        std::string GetSchool()
        {
            return school_;
        }

        std::string& GetMsg()
        {
            return msg_;
        }

        uint32_t& GetUserId()
        {
            return userid_;
        }

        void SetNickName(std::string& nickname)
        {
            nickname_ = nickname;
        }

        void SetSchool(std::string& school)
        {
            school_ = school;
        }

        void SetMsg(std::string& msg)
        {
            msg_ = msg;
        }

        void SetUserId(uint32_t& userid)
        {
            userid_ = userid;
        }
    private:
        std::string nickname_;
        std::string school_;
        std::string msg_;
        uint32_t userid_;
};
