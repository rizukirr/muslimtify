#include "../include/cli.h"
#include <curl/curl.h>

int main(int argc, char **argv) {
    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Parse command line arguments
    CliArgs args = cli_parse(argc, argv);
    
    // Execute command
    int result = cli_execute(&args);
    
    // Cleanup
    curl_global_cleanup();
    
    return result;
}
