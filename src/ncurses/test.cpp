#include <unistd.h>
#include <ncurses.h>

int main()
{
    //开始虚拟化界面
    initscr();
    //0表示界面上不显示光标
    curs_set(0);
    //设置界面的大小
    WINDOW* header = newwin(10, 10, 0, 0);
    //在输出设备上展示出来
    wrefresh(header);
    //设置边框
    box(header, 0, 0);
    sleep(4);
    //结束虚拟化界面
    endwin();
    return 0;
}
