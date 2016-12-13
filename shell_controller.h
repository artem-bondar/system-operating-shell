#ifndef __SHELL_CONTROLLER_H__
#define __SHELL_CONTROLLER_H__

#include <sys/types.h>

#include "miscellaneous_stuff.h"

#define MIN_TERMINAL_WIDTH 30
#define MIN_TERMINAL_HEIGHT 9

#define DEFAULT 0
#define NONCANONICAL 1

#define NO_FINISHED_PROCESS 0
#define GOT_FINISHED_PROCESS 1

typedef enum shell_enviroment_values_names
{
	NUMBER_OF_RECEIVED_SHELL_ARGUMENTS,
	LAST_FINISHED_PROCESS_STATUS,
	USERNAME,
	HOME_DIRECTORY_PATH,
	SHELL_EXECUTABLE_FILE_PATH,
	USER_ID,
	CURRENT_DIRECTORY_PATH,
	SHELL_PROCESS_ID
} shell_enviroment_values_names;

typedef struct shell_environments
{
	char executable_file_path[BUFFER_SIZE];
	char current_directory[BUFFER_SIZE];
	char *username, *home_directory;
	uid_t uid;
	pid_t pid;
	unsigned int arguments_received;
	int finished_any_process_status, last_foreground_process_status;
} shell_environment;

typedef struct environment_detection_errors
{
	errors_shell terminal_attributes, shell_path, environmental_values, current_directory;
} environment_detection_errors;

environment_detection_errors detect_environment(shell_environment *environmental_values, run_mode *mode, stderr_mode *error_output);
void renew_environment_values(shell_environment *environmental_values);
errors_shell check_and_complete_environment_values(shell_environment *environmental_values, environment_detection_errors detected_errors, run_mode mode, stderr_mode error_output, unsigned int shell_arguments_amount, char *default_shell_path);
errors_shell get_terminal_size(unsigned short *screen_width, unsigned short *screen_height);
errors_shell change_terminal_input_mode_to(const unsigned int mode);
void universal_signal_handler(int signal);
void exit_with_free(errors_executor exit_code);
errors_executor poll_for_finished_processes(run_mode mode, stderr_mode error_output, unsigned int *amount_of_alive_processes);

#endif /* __SHELL_CONTROLLER_H__ */