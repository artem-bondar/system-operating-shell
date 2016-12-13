#define _BSD_SOURCE /* usleep() macro requirement */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "interface_controller.h"
#include "shell_controller.h"

const char *title[TITLE_SIZE] =
{
	"  _____    ____     _____   _ ",
	" / ____|  / __ \\   / ____| / |",
	"| (___   | |  | | | (___   | |",
	" \\___ \\  | |  | |  \\___ \\  |_/",
	" ____) | | |__| |  ____) |  _ ",
	"|_____/   \\____/  |_____/  |_|\n",
	"Created by Artem Bondar © 2016"
};

const char *error_messages[AMOUNT_OF_ERROR_MESSAGES] =
{
 /* errors_global */
	"no errors ocurred",
	"memory allocation error",
 /* errors_stream */
	"can't access stdin for read",
	"can't access stdout for write",
	"can't access stderr for write",
	"stdin was closed, command was read",
	"stdin was closed, no command was read",
	"empty command used",
  /* errors_shell */
	"can't access password database to get environment values",
	"failed to set environmental values to terminal",
	"can't get path of shell executable file",
	"can't get current directory path",
	"failed to get terminal size",
	"failed to get terminal attributes",
	"failed to set terminal attributes",
	"terminal size is too small to run this program",
  /* errors_replacement */
	"trying to substitute unexisting shell argument",
	"retrieved environment value is absent",
	"trying to substitute empty argument",
	"requested unexisting history command",
 /* errors_parser */
	"incorrect shell control characters syntax",
	"invalid control character was used",
	"program is required, but is missing",
	"stdin file is missing",
	"stdout file is missing",
	"tried to designate second stdin file",
	"tried to designate second stdout file",
	"incorrect usage of & symbol",
	"incorrect arguments order",
	"incorrect quotes formatting",
	"command has nothing to execute",
 /* errors_executor */
	"missing required argument",
	"too many arguments received",
	"unrecognized key was used",
	"extra stdin stream",
	"extra stdout stream",
	"second arguments is required to be a number",
	"job with this number is unexisting or already terminated",
	"aborted trying to create a pipe",
	"aborted trying to open a file",
	"aborted trying to fork a new process",
	"couldn't close a stream",
	"couldn't redirect a stream",
	"error occured when was waiting for child to terminate",
	"failed trying to launch an external process",
	"failed trying to change process group",
	"failed to change directory",
	"failed to change terminal control group",
	"help file is missing or inaccessible",
 /* errors_signals */
	"failed to attach signal handler",
	"SIGINT was received, shell terminated",
	"a process became an orphan",
	"undefined signal received"
};

const char *built_in_commands_names[AMOUNT_OF_BUILT_IN_COMMANDS] =
{
	"undetected",
	"external",
	"cd",
	"pwd",
	"jobs",
	"fg",
	"bg",
	"exit",
	"history",
	"mcat",
	"msed",
	"mgrep",
	"help"
};

void printf_colored(char const *string, const color foreground, const color background)
{
	printf("\033[%d;%dm%s\033[0;37;40m", 30 + foreground, 40 + background, string);
}

void printf_colored_attributed(char const *string, const color foreground, const color background, unsigned int attributes_number, ...)
{
	unsigned int attribute;

	if (attributes_number)
	{
		va_list argument_list;
		va_start(argument_list, attributes_number);
		while (attributes_number)
		{
			attribute = va_arg(argument_list, unsigned int);
			printf("\033[%um", attribute);
			attributes_number--;
		}
		va_end(argument_list);
	}
	printf_colored(string, foreground, background);
}

void show_loading_screen(const unsigned short indent, const unsigned short vertical_indent)
{
	int i, j;
	color load_color;

	printf("\033[?25l\033[2J\033[1;0H\033[%dB", vertical_indent);
	for (i = 0; i < TITLE_SIZE; i++)
	{
		printf("%*s", indent, "");
		printf_colored(title[i], YELLOW, BLACK);
		printf("\n");
	}
	putchar('\n');
	for (i = 0; i <= 100; i++)
	{
		printf("%*s", indent, "");
		load_color = (i > 33) ? (i > 66) ? GREEN : YELLOW : RED;
		for (j = i / 4; j > 0; j--)
		{
			printf_colored("█", load_color, BLACK);
		}
		if (i % 2 == 1)
			printf_colored("▌", load_color, BLACK);
		printf("%3d%%\r", i);
		usleep(30000);
	}
	usleep(100000);
	printf("%*s", indent, "");
	printf_colored("█████████████████████████ DONE \n", GREEN, BLACK);
	usleep(500000);
	printf("\033[;H\033[J\033[?25h");
}

void send_error_message_and_terminate_if_critical(unsigned int error_code, run_mode mode, stderr_mode error_output)
{
	fprintf(stderr, "%sshell: %s \n%s", (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[1;3;31;40m" : "",
		    error_messages[error_code], (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[0;37;40m" : "");
	switch (error_code)
	{
	case ALLOCATION_ERROR:
	case FAILED_TO_READ_FROM_STDIN:
	case FAILED_TO_WRITE_TO_STDOUT:
	case FAILED_TO_GET_TERMINAL_SIZE:
	case FAILED_TO_SET_TERMINAL_ATTRIBUTES:
	case TERMINAL_SIZE_TOO_SMALL:
	case FAILED_TO_ATTACH_SIGNAL_HANDLER:
	case SIGINT_RECEIVED:
		exit_with_free(error_code);
	default:
		break;
	}
	if (ferror(stderr))
		exit_with_free(FAILED_TO_WRITE_TO_STDERR);
}

errors_stream show_input_invite(char *username)
{
	printf_colored(username, YELLOW, BLACK);
	putchar('@');
	printf_colored_attributed("shell: ", GREEN, BLACK, 1, ITALIC);
	return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
}

errors_stream send_launch_error_message(char *process_name, run_mode mode, stderr_mode error_output)
{
	fprintf(stderr, "%sshell: %s: command not found \n%s", (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[1;3;31;40m" : "",
											 process_name, (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[0;37;40m" : "");
	return ferror(stdout) ? FAILED_TO_WRITE_TO_STDERR : NO_ERROR;
}

void send_built_in_programs_error(const char *program_name, unsigned int error_code, run_mode mode, stderr_mode error_output)
{
	fprintf(stderr, "%sshell: %s: %s \n%s", (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[1;3;31;40m" : "",
  program_name, error_messages[error_code], (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[0;37;40m" : "");
	if (ferror(stderr))
		exit_with_free(FAILED_TO_WRITE_TO_STDERR);
}

errors_stream try_to_report_finished_job(jobs_cartulary *cartulary, pid_t pid, run_mode mode, stderr_mode error_output)
{
	cartulary_record *current_record = cartulary->newest_record;
	int i, j;

	while (current_record != NULL)
	{
		for (i = 0; i < current_record->number_of_processes; i++)
			if (pid == current_record->processes[i].pid)
			{
				for (j = 0; j < current_record->number_of_processes; j++)
					if (current_record->processes[j].status != TERMINATED)
						return NO_ERROR;
				fprintf(stderr, "%sshell: %s: finished \n%s", (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[1;3;33;40m" : "",
									current_record->job_name, (error_output == TO_TERMINAL && (mode == INTERACTIVE || mode == OUTPUT_ONLY)) ? "\033[0;37;40m" : "");
				return ferror(stderr) ? FAILED_TO_WRITE_TO_STDERR : NO_ERROR;;
			}
		current_record = current_record->older_record;
	}
	return NO_ERROR;
}