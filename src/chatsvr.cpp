#include "chatsvr.hpp"

int main()
{
    ChatSvr cs;
    cs.Init();
    cs.Start();
    while(1)
    {
        sleep(1);
    }
    return 0;
}
