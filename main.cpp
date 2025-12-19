#include "yobot_paint.h"

int main(int argc, char const *argv[])
{
    yobot::paint::getInstance()
        .preparePanel()
        .refreshPanelIcons({ 312501,316600,300701,316102,302600 })
        .save()
        .show();
    return 0;
}
