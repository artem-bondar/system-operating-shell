#ifndef __INTERFACE_CONTROLLER_H__
#define __INTERFACE_CONTROLLER_H__

#include "jobs_executor.h"

#define AMOUNT_OF_BUILT_IN_COMMANDS 13

typedef enum color
{
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE
} color;

typedef enum graphic_rendering_attributes
{
	RESET,
	BOLD,
	ITALIC = 3,
	UNDERLINE,
	BLINK
} graphic_rendering_attributes;

extern const char *title[TITLE_SIZE];
extern const char *built_in_commands_names[AMOUNT_OF_BUILT_IN_COMMANDS];

void printf_colored(char const *string, const color foreground, const color background);
void printf_colored_attributed(char const *string, const color foreground, const color background, unsigned int attributes_number, ...);
void show_loading_screen(const unsigned short indent, const unsigned short vertical_indent);
void send_error_message_and_terminate_if_critical(unsigned int error_code, run_mode mode, stderr_mode error_output);
void send_built_in_programs_error(const char *program_name, unsigned int error_code, run_mode mode, stderr_mode error_output);
errors_stream show_input_invite(char *username);
errors_stream send_launch_error_message(char *process_name, run_mode mode, stderr_mode error_output);
errors_stream try_to_report_finished_job(jobs_cartulary *cartulary, pid_t pid, run_mode mode, stderr_mode error_output);

#endif /* __INTERFACE_CONTROLLER_H__ */