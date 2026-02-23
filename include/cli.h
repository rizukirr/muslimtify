#ifndef CLI_H
#define CLI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CMD_SHOW,      // Default: show prayer times
    CMD_CHECK,     // Check and notify if prayer time
    CMD_NEXT,      // Show next prayer
    CMD_CONFIG,    // Config management
    CMD_LOCATION,  // Location management
    CMD_ENABLE,    // Enable prayer
    CMD_DISABLE,   // Disable prayer
    CMD_LIST,      // List prayer status
    CMD_REMINDER,  // Manage reminders
    CMD_DAEMON,    // Daemon management
    CMD_VERSION,   // Show version
    CMD_HELP,      // Show help
    CMD_UNKNOWN
} CliCommand;

typedef struct {
    CliCommand command;
    char *subcommand;
    int argc;
    char **argv;
} CliArgs;

/**
 * Parse command line arguments
 */
CliArgs cli_parse(int argc, char **argv);

/**
 * Execute CLI command
 * Returns: 0 on success, non-zero on error
 */
int cli_execute(const CliArgs *args);

/**
 * Print help message
 */
void cli_print_help(const char *command);

#ifdef __cplusplus
}
#endif

#endif // CLI_H
