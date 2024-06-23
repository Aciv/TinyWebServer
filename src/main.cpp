#include <unistd.h>
#include "server/webServer.h"

int main(int argc, char * argv[]){

    cWebServer server(1342, 3, 60000, 6, 
                    true, 1, 1024);
    server.Start();
    return 0;
}
