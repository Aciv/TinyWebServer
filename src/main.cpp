#include <unistd.h>
#include "server/webServer.h"

int main(int argc, char * argv[]){

    cWebServer server(1342, 3, 60000, 6, 
                    3306, "root", "dao1815", "webserver", /* Mysql≈‰÷√ */
                    12, true, 0, 1024);
    server.Start();
    return 0;
}
