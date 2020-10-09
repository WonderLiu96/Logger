/**
 * Created by wonder on 2020/10/9.
 **/
#include "log.h"

int main(int argc, char *argv[]) {
    using namespace wonder;
    Log::Instance()->init();
    LOG_INFO("%s","This is info.");
    LOG_WARN("%s","WARN");
}
