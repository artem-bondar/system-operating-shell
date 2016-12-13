#ifndef __STREAM_HANDLER_H__
#define __STREAM_HANDLER_H__

#include "jobs_executor.h"

typedef enum stdin_reading_statuss
{
	READING,
	READ_FINISH,
	BUFFER_END,
	UP_ARROW,
	DOWN_ARROW,
	LEFT_ARROW,
	RIGHT_ARROW,
	EOF_RECEIVED
} stdin_reading_status;

errors_stream get_shell_command_interactively(jobs_history *history);
errors_stream get_shell_command_non_interactively(jobs_history *history);
errors_stream get_line_from_stdin(char **line);

#endif /* __STREAM_HANDLER_H__ */