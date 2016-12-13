#include <string.h>
#include <signal.h>

#include "interface_controller.h"
#include "shell_controller.h"
#include "stream_handler.h"
/* 
declared as global variables
for handler purposes */
jobs_cartulary *cartulary = NULL;
jobs_history *history = NULL;

int main(int argc, char **argv)
{
	unsigned short screen_width, screen_height, indent, vertical_indent, possible_error;
	unsigned int amount_of_alive_processes = 0;
	shell_environment environmental_values;
	environment_detection_errors detected_errors;
	exit_statuses exit_enquiry = NOT_RECEIVED;
	errors_stream input_result;
	errors_parser parse_result;
	stderr_mode error_output;
	run_mode mode;

	detected_errors = detect_environment(&environmental_values, &mode, &error_output);
	if ((possible_error = check_and_complete_environment_values(&environmental_values, detected_errors, mode, error_output, argc, argv[0])))
		return possible_error;

	if (mode == INTERACTIVE)
	{
		if (argc == 1 || (argc >= 2 && strcmp(argv[1], "fast")))
		{
			if ((possible_error = get_terminal_size(&screen_width, &screen_height)))
				send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
			calculate_indents(&indent, &vertical_indent, screen_width, screen_height);
			show_loading_screen(indent, vertical_indent);
		}
	}

	if ((signal(SIGINT, SIG_IGN)) == SIG_ERR)
		send_error_message_and_terminate_if_critical(FAILED_TO_ATTACH_SIGNAL_HANDLER, mode, error_output);
	if ((signal(SIGTSTP, SIG_IGN)) == SIG_ERR)
		send_error_message_and_terminate_if_critical(FAILED_TO_ATTACH_SIGNAL_HANDLER, mode, error_output);
	if ((signal(SIGTTOU, SIG_IGN)) == SIG_ERR)
		send_error_message_and_terminate_if_critical(FAILED_TO_ATTACH_SIGNAL_HANDLER, mode, error_output);
 /* blocking SIGINT & SIGTSTP without any fear because their ^C, ^Z behaviour is emulated while reading
	command from stdin. SIGTTOU is blocked too not to stop shell process while calling tcsetpgrp() on it */

	if ((possible_error = initialize_history(&history)))
		send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
	if ((possible_error = initialize_cartulary(&cartulary)))
		send_error_message_and_terminate_if_critical(possible_error, mode, error_output);

	while (exit_enquiry != RECEIVED)
	{
		if (mode == INTERACTIVE)
		{
			if ((possible_error = show_input_invite(environmental_values.username)))
				send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
			if ((possible_error = change_terminal_input_mode_to(NONCANONICAL)))
				send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
			input_result = get_shell_command_interactively(history);
		}
		else
			input_result = get_shell_command_non_interactively(history);
		if (mode == INTERACTIVE)
		{
			if ((possible_error = change_terminal_input_mode_to(DEFAULT)))
				send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
		}
		if (input_result == EOF_AND_EMPTY_COMMAND)
			break;
		if (input_result == EOF_BUT_COMMAND_READ)
			exit_enquiry = RECEIVED;
		if (input_result != EMPTY_COMMAND)
		{
			if (input_result && input_result != EOF_BUT_COMMAND_READ)
				send_error_message_and_terminate_if_critical(input_result, mode, error_output);
			parse_result = parse_record_text_to_executable_job(history, history->newest_record, argv, argc);
			if (parse_result && parse_result != NOTHING_TO_EXECUTE)
				send_error_message_and_terminate_if_critical(parse_result, mode, error_output);
			else
				if ((possible_error = execute_command(history->newest_record, &amount_of_alive_processes, &exit_enquiry, mode, error_output)))
					send_error_message_and_terminate_if_critical(possible_error, mode, error_output);
		}
		if (amount_of_alive_processes)
			poll_for_finished_processes(mode, error_output, &amount_of_alive_processes);
		check_cartulary_for_stopped_conveyor(cartulary);
		check_cartulary_for_jobs_statuses_changes(cartulary);
		check_cartulary_for_terminated_jobs_and_remove_if_found(cartulary);
		renew_environment_values(&environmental_values);
	}
	free_history(history);
	free_cartulary(cartulary);
	return NO_ERROR;
}