#define _BSD_SOURCE /* readlink() macro requirement */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pwd.h>

#include "shell_controller.h"
#include "interface_controller.h"
#include "jobs_executor.h"

static struct termios default_terminal_attributes, noncanonical_terminal_attributes;

environment_detection_errors detect_environment(shell_environment *environmental_values, run_mode *mode, stderr_mode *error_output)
{
	char path_to_info[BUFFER_SIZE];
	struct passwd *password_file_entry;
	environment_detection_errors errors_statuses;

	environmental_values->finished_any_process_status = NO_FINISHED_PROCESS;
	environmental_values->uid = getuid();
	environmental_values->pid = getpid();
 /* manual says these functions are always successful */

	if (isatty(0))
	{
		if (isatty(1))
			*mode = INTERACTIVE;
		else
			*mode = INPUT_ONLY;
	}
	else
	{
		if (isatty(1))
			*mode = OUTPUT_ONLY;
		else
			*mode = AUTONOMOUS;
	}
	if (isatty(2))
		*error_output = TO_TERMINAL;
	else
		*error_output = TO_FILE;
	if (*mode == INTERACTIVE)
	{
		if (tcgetattr(0, &default_terminal_attributes))
			errors_statuses.terminal_attributes = FAILED_TO_GET_TERMINAL_ATTRIBUTES;
		else
		{
		 /* only this part is essential for work, in another errors default values are set */
			errors_statuses.terminal_attributes = NO_ERROR;
			memcpy(&noncanonical_terminal_attributes, &default_terminal_attributes, sizeof(struct termios));
			noncanonical_terminal_attributes.c_lflag &= ~(ECHO | ICANON | ISIG);
			noncanonical_terminal_attributes.c_cc[VMIN] = 1;
		}
	}
	else
		errors_statuses.terminal_attributes = NO_ERROR;

	sprintf(path_to_info, "/proc/%d/exe", environmental_values->pid);
	memset(environmental_values->executable_file_path, 0, BUFFER_SIZE);
	errors_statuses.shell_path = (readlink(path_to_info, environmental_values->executable_file_path, BUFFER_SIZE) == -1) ? FAILED_TO_GET_SHELL_PATH : NO_ERROR;
	
	errors_statuses.environmental_values = ((password_file_entry = getpwuid(environmental_values->uid)) == NULL) ? FAILED_TO_GET_ENVIRONMENTAL_VALUES : NO_ERROR;
	environmental_values->username = password_file_entry->pw_name;
	environmental_values->home_directory = password_file_entry->pw_dir;

	errors_statuses.current_directory = (getcwd(environmental_values->current_directory, BUFFER_SIZE) == NULL) ? FAILED_TO_GET_CURRENT_DIRECTORY_PATH : NO_ERROR;

	return errors_statuses;
}

void renew_environment_values(shell_environment *environmental_values)
{
	char new_directory[BUFFER_SIZE];
	struct passwd *password_file_entry;

	environmental_values->uid = getuid();
	memset(environment_values_replacement_patterns[USER_ID], 0, BUFFER_SIZE);
	sprintf(environment_values_replacement_patterns[USER_ID], "%d", environmental_values->uid);
	if ((password_file_entry = getpwuid(environmental_values->uid)) != NULL)
	{
		environmental_values->username = password_file_entry->pw_name;
		environmental_values->home_directory = password_file_entry->pw_dir;
		memset(environment_values_replacement_patterns[USER_ID], 0, BUFFER_SIZE);
		memset(environment_values_replacement_patterns[HOME_DIRECTORY_PATH], 0, BUFFER_SIZE);
		sprintf(environment_values_replacement_patterns[USERNAME], "%s", environmental_values->username);
		sprintf(environment_values_replacement_patterns[HOME_DIRECTORY_PATH], "%s", environmental_values->home_directory);
	}
	if (getcwd(new_directory, BUFFER_SIZE) != NULL)
	{
		memset(environmental_values->current_directory, 0, BUFFER_SIZE);
		strcpy(environmental_values->current_directory, new_directory);
		memset(environment_values_replacement_patterns[CURRENT_DIRECTORY_PATH], 0, BUFFER_SIZE);
		sprintf(environment_values_replacement_patterns[CURRENT_DIRECTORY_PATH], "%s", environmental_values->current_directory);
	}
}

errors_shell check_and_complete_environment_values(shell_environment *environmental_values, environment_detection_errors detected_errors, run_mode mode, stderr_mode error_output, unsigned int shell_arguments_amount, char *default_shell_path)
{
	if (detected_errors.terminal_attributes)
	{
		send_error_message_and_terminate_if_critical(FAILED_TO_GET_TERMINAL_ATTRIBUTES, mode, error_output);
		return FAILED_TO_GET_TERMINAL_ATTRIBUTES;
	}
	if (detected_errors.shell_path)
	{
		send_error_message_and_terminate_if_critical(FAILED_TO_GET_SHELL_PATH, mode, error_output);
		sprintf(environmental_values->executable_file_path, "%s", default_shell_path);
	}
	if (detected_errors.environmental_values)
	{
		send_error_message_and_terminate_if_critical(FAILED_TO_GET_ENVIRONMENTAL_VALUES, mode, error_output);
		sprintf(environmental_values->username, "username");
		sprintf(environmental_values->home_directory, "home directory");
	}
	if (detected_errors.current_directory)
	{
		send_error_message_and_terminate_if_critical(FAILED_TO_GET_CURRENT_DIRECTORY_PATH, mode, error_output);
		sprintf(environmental_values->current_directory, "current directory");
	}
	environmental_values->arguments_received = shell_arguments_amount;

	sprintf(environment_values_replacement_patterns[NUMBER_OF_RECEIVED_SHELL_ARGUMENTS], "%d", shell_arguments_amount);
	sprintf(environment_values_replacement_patterns[LAST_FINISHED_PROCESS_STATUS], "0");
	sprintf(environment_values_replacement_patterns[USERNAME], "%s", environmental_values->username);
	sprintf(environment_values_replacement_patterns[HOME_DIRECTORY_PATH], "%s", environmental_values->home_directory);
	sprintf(environment_values_replacement_patterns[SHELL_EXECUTABLE_FILE_PATH], "%s", environmental_values->executable_file_path);
	sprintf(environment_values_replacement_patterns[USER_ID], "%d", environmental_values->uid);
	sprintf(environment_values_replacement_patterns[CURRENT_DIRECTORY_PATH], "%s", environmental_values->current_directory);
	sprintf(environment_values_replacement_patterns[SHELL_PROCESS_ID], "%d", environmental_values->pid);	
	
	if (setenv("#", environment_values_replacement_patterns[NUMBER_OF_RECEIVED_SHELL_ARGUMENTS], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("USER", environment_values_replacement_patterns[USERNAME], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("HOME", environment_values_replacement_patterns[HOME_DIRECTORY_PATH], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("SHELL", environment_values_replacement_patterns[SHELL_EXECUTABLE_FILE_PATH], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("UID", environment_values_replacement_patterns[USER_ID], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("PWD", environment_values_replacement_patterns[CURRENT_DIRECTORY_PATH], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;
	if (setenv("PID", environment_values_replacement_patterns[SHELL_PROCESS_ID], 1))
		return FAILED_TO_SET_ENVIRONMENTAL_VALUES;

	return NO_ERROR;
}

errors_shell get_terminal_size(unsigned short *screen_width, unsigned short *screen_height)
{
	struct winsize window_sizes;

	if (ioctl(0, TIOCGWINSZ, &window_sizes) && ioctl(1, TIOCGWINSZ, &window_sizes))
		return FAILED_TO_GET_TERMINAL_SIZE;
	if (window_sizes.ws_col < MIN_TERMINAL_WIDTH)
	 /* forces user to use terminal width bigger or equal to load screen width */
		return TERMINAL_SIZE_TOO_SMALL;
	if (window_sizes.ws_row < MIN_TERMINAL_HEIGHT)
	 /* forces user to use terminal height bigger or equal to load screen height */
		return TERMINAL_SIZE_TOO_SMALL;

	*screen_width = window_sizes.ws_col;
	*screen_height = window_sizes.ws_row;

	return NO_ERROR;
}

errors_shell change_terminal_input_mode_to(const unsigned int mode)
{
	if (mode == DEFAULT)
		if (tcsetattr(0, TCSANOW, &default_terminal_attributes))
			return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	if (mode == NONCANONICAL)
		if (tcsetattr(0, TCSANOW, &noncanonical_terminal_attributes))
			return FAILED_TO_SET_TERMINAL_ATTRIBUTES;
	return NO_ERROR;
}

void universal_signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		exit_with_free(SIGINT_RECEIVED);
	case SIGHUP:
		exit_with_free(A_PROCESS_BECOME_AN_ORPHAN);
	default:
		exit_with_free(UNDEFINED_SIGNAL_RECEIVED);
	}
}

void exit_with_free(errors_executor exit_code)
{
	if (history != NULL)
		free_history(history);
	if (cartulary != NULL)
		free_cartulary(cartulary);
	exit(exit_code);
}

errors_executor poll_for_finished_processes(run_mode mode, stderr_mode error_output, unsigned int *amount_of_alive_processes)
{
	pid_t pid;
	int exit_status;
	process_statuses change_type;

	do
	{
		if ((pid = waitpid(-1, &exit_status, WNOHANG | WUNTRACED)) == -1)
			return FAILED_TO_WAIT_CHILD_STATUS;
		if (pid)
		{
			change_type = (WIFSTOPPED(exit_status)) ? STOPPED : TERMINATED;
			update_process_status(cartulary, pid, change_type, exit_status);
			if (change_type == TERMINATED)
			{
				amount_of_alive_processes--;
				if (WEXITSTATUS(exit_status) == FAILED_TO_LAUNCH_A_PROCESS)
					if (send_launch_error_message(retrieve_process_name_by_pid(cartulary, pid), mode, error_output))
						return FAILED_TO_WRITE_TO_STDERR;
				try_to_report_finished_job(cartulary, pid, mode, error_output);
			}
		}
	} while (amount_of_alive_processes && pid);
	return NO_ERROR;
}