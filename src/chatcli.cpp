#include "chatcli.hpp"
#include "window.hpp"

int Menu()
{
    std::cout << "------------------------------" << std::endl;
    std::cout << "|          1.register        |" << std::endl;
    std::cout << "|          2.login           |" << std::endl;
    std::cout << "|          3.exit            |" << std::endl;
    std::cout << "------------------------------" << std::endl;
    int select = -1;
    std::cout << "Please enter a select:";
    fflush(stdout);
    std::cin >> select;
    return select;
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout << "./chatcli [ip]" << std::endl;
        return 0;
    }

    ChatCli* cc = new ChatCli(argv[1]);
    cc->Init();
    while(1)
    {
        int select = Menu();
        if(select == 1)
        {
            //注册
            if(!cc->Register())
            {
                LOG(ERROR, "Register failed, Please Try again!") << std::endl; 
                continue;
            }
            else 
            {
                LOG(INFO, "Register success, Please login!") << std::endl;
                continue;
            }
        }
        else if(select == 2)
        {
            //登录
            if(!cc->Login())
            {
                LOG(ERROR, "Login failed, Please try again!") << std::endl; 
                continue;
            }
            else 
            {
                LOG(INFO, "Login success, Please chat online!") << std::endl;
                Window* window = new Window();
                window->Start(cc);
                continue;
            }
        }
        else if(select == 3)
        {
            //退出
            break;
        }
        else 
        {
            //输入非法
            LOG(ERROR, "Select is not effective, Please try again!") << std::endl;
            continue;
        }
    }
    delete cc;
    return 0;
}
