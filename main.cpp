#include "yobot_paint.h"

int main(int argc, char const *argv[])
{
    yobot::paint::getInstance().draw();
    getchar();
    return 0;
}
