#pragma once 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <vector>

#include "chatcli.hpp"
#include "jsonmessage.hpp"

class Window;

class Param 
{
    public:
        Param(Window* winp, int threadnum, ChatCli* chatcli)
        {
            winp_ = winp;
            threadnum_ = threadnum;
            chatcli_ = chatcli;
        }

        Window* GetWin()
        {
            return winp_;
        }

        int& GetThreadNum()
        {
            return threadnum_;
        }

        ChatCli* GetChatCli()
        {
            return chatcli_;
        }
    private:
        Window* winp_;
        int threadnum_;
        ChatCli* chatcli_;
};

class Window
{
    public:
        Window()
        {
            header_ = NULL;
            output_ = NULL;
            userlist_ = NULL;
            input_ = NULL;
            threads_.clear();
            pthread_mutex_init(&lock_, NULL);
            initscr();
            curs_set(0);
        }

        ~Window()
        {
            if(header_)
            {
                delwin(header_);
            }

            if(output_)
            {
                delwin(output_);
            }

            if(userlist_)
            {
                delwin(userlist_);
            }

            if(input_)
            {
                delwin(input_);
            }

            endwin();
            pthread_mutex_destroy(&lock_);
        }

        void DrawHeader()
        {
            int lines = LINES / 5;
            int cols = COLS;
            int start_y = 0;
            int start_x = 0;
            header_ = newwin(lines, cols, start_y, start_x);
            box(header_, 0, 0);
            pthread_mutex_lock(&lock_);
            wrefresh(header_);
            pthread_mutex_unlock(&lock_);
        }

        void DrawOutput()
        {
            int lines = (LINES * 3) / 5;
            int cols = (COLS * 3) / 4;
            int start_y = LINES / 5;
            int start_x = 0;
            output_ = newwin(lines, cols, start_y, start_x);
            box(output_, 0, 0);
            pthread_mutex_lock(&lock_);
            wrefresh(output_);
            pthread_mutex_unlock(&lock_);
        }

        void DrawUserList()
        {
            int lines = (LINES * 3) / 5;
            int cols = COLS / 4;
            int start_y = LINES / 5;
            int start_x = (COLS * 3) / 4;
            userlist_ = newwin(lines, cols, start_y, start_x);
            box(userlist_, 0, 0);
            pthread_mutex_lock(&lock_);
            wrefresh(userlist_);
            pthread_mutex_unlock(&lock_);
        }

        void DrawInput()
        {
            int lines = LINES / 5;
            int cols = COLS;
            int start_y = (LINES * 4) / 5;
            int start_x = 0;
            input_ = newwin(lines, cols, start_y, start_x);
            box(input_, 0, 0);
            pthread_mutex_lock(&lock_);
            wrefresh(input_);
            pthread_mutex_unlock(&lock_);
        }

        void GetStringFronWin(WINDOW* window, std::string& data)
        {
            char buf[1024];
            memset(buf, '\0', sizeof(buf));
            wgetnstr(window, buf, sizeof(buf) - 1);
            data.assign(buf, strlen(buf));
        }

        void PutSrtingToWin(WINDOW* window, int y, int x, std::string& msg)
        {
            mvwaddstr(window, y, x, msg.c_str());
            pthread_mutex_lock(&lock_);
            wrefresh(window);
            pthread_mutex_unlock(&lock_);
        }

        static void RunHeader(Window* win)
        {
            //1.绘制窗口
            //2.展示欢迎语
            int y, x;
            size_t pos = 1;
            int flag = 0; //0:向右输出，1:向左输出
            std::string msg = "Welcome to out chat hall";
            while(1)
            {
                win->DrawHeader();
                //getmaxyx函数返回当前窗口最大行数存储在y中，返回最大列数存储在x中
                getmaxyx(win->header_, y, x);
                win->PutSrtingToWin(win->header_, y / 2, pos, msg);
                if(pos < 2)
                {
                    flag = 0;
                }
                else if(pos > x - msg.size() - 2)
                {
                    flag = 1;
                }

                if(flag == 0)
                {
                    pos++;
                }
                else if(flag == 1)
                {
                    pos--;
                }
                sleep(1);
            }
        }

        static void RunOutput(Window* win, ChatCli* cc)
        {
            std::string recv_msg;
            JsonMessage msg;
            win->DrawOutput();
            int y, x;
            getmaxyx(win->output_, y, x);
            int line = 1;
            while(1)
            {
                cc->RecvMsg(recv_msg);
                //反序列化
                msg.Deserialization(recv_msg);
                //展示数据 昵称-学校：消息
                std::string show_msg;
                show_msg += msg.GetNickName();
                show_msg += "-";
                show_msg += msg.GetSchool();
                show_msg += ": ";
                show_msg += msg.GetMsg();
                if(line > y - 2)
                {
                    line = 1;
                    win->DrawOutput();
                }

                win->PutSrtingToWin(win->output_, line, 1, show_msg);
                line++;

                std::string user_info;
                user_info += msg.GetNickName();
                user_info += "-";
                user_info += msg.GetSchool();
                cc->PushUser(user_info);
            }
        }

        static void RunInput(Window* win, ChatCli* cc)
        {
            JsonMessage msg;
            msg.SetNickName(cc->GetMySelf().nickname_);
            msg.SetSchool(cc->GetMySelf().school_);
            msg.SetUserId(cc->GetMySelf().userid_);

            //用户输入发送的消息
            std::string user_msg; 
            //序列化完成之后的消息
            std::string send_msg;

            std::string tips = "Please enter: ";
            while(1)
            {
                win->DrawInput();
                win->PutSrtingToWin(win->input_, 1, 1, tips);
                //从窗口当中获取数据，放到send_msg中
                win->GetStringFronWin(win->input_, user_msg); 

                msg.SetMsg(user_msg);
                msg.Serialize(send_msg);         
                cc->SendMsg(send_msg); 
            }
        }

        static void RunUserList(Window* win, ChatCli* cc)
        {
            int y, x;
            while(1)
            {
                win->DrawUserList();
                int line = 1;
                getmaxyx(win->userlist_, y, x);
                std::vector<std::string> userlist = cc->GetOnlineUserList();
                for(auto& iter:userlist)
                {
                    win->PutSrtingToWin(win->userlist_, line, 1, iter);
                    line++;
                }
                sleep(1);
            }
        }

        static void* DrawWindow(void* arg)
        {
            Param* pm = (Param*)arg;
            Window* win = pm->GetWin(); 
            ChatCli* cc = pm->GetChatCli();
            int threadnum = pm->GetThreadNum();
            switch(threadnum)
            {
                case 0:
                    RunHeader(win);
                    break;
                case 1:
                    RunOutput(win, cc);
                    break;
                case 2:
                    RunInput(win, cc);
                    break;
                case 3:
                    RunUserList(win, cc);
                    break;
                default:
                    break;
            }
            delete pm;
            return NULL;
        }

        void Start(ChatCli* chatcli)
        {
            int i = 0;
            pthread_t tid;
            for(; i < 4; i++)
            {
                Param* pm = new Param(this, i, chatcli);
                int ret = pthread_create(&tid, NULL, DrawWindow, (void*)pm);
                if(ret < 0)
                {
                    printf("Create thread failed\n");
                    exit(1);
                }
                threads_.push_back(tid);
            }
            
            for(i = 0; i < 4; i++)
            {
                pthread_join(threads_[i], NULL);
            }
        }
    private:
        WINDOW* header_;
        WINDOW* output_;
        WINDOW* userlist_;
        WINDOW* input_;
        std::vector<pthread_t> threads_;
        pthread_mutex_t lock_;
};
