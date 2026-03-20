#include "../include/cli.h"
#include <curl/curl.h>

int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int result = cli_run(argc, argv);

    curl_global_cleanup();

    return result;
}
