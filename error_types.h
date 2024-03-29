#ifndef __ERROR_TYPES_H__
#define __ERROR_TYPES_H__

#define AMOUNT_OF_ERROR_MESSAGES 53

typedef enum errors_global
{
	NO_ERROR,
	ALLOCATION_ERROR
} errors_global;

typedef enum errors_stream
{
	FAILED_TO_READ_FROM_STDIN = 2,
	FAILED_TO_WRITE_TO_STDOUT,
	FAILED_TO_WRITE_TO_STDERR,
	EOF_BUT_COMMAND_READ,
	EOF_AND_EMPTY_COMMAND,
	EMPTY_COMMAND
} errors_stream;

typedef enum errors_shell
{
	FAILED_TO_GET_ENVIRONMENTAL_VALUES = 8,
	FAILED_TO_SET_ENVIRONMENTAL_VALUES,
	FAILED_TO_GET_SHELL_PATH,
	FAILED_TO_GET_CURRENT_DIRECTORY_PATH,
	FAILED_TO_GET_TERMINAL_SIZE,
	FAILED_TO_GET_TERMINAL_ATTRIBUTES,
	FAILED_TO_SET_TERMINAL_ATTRIBUTES,
	TERMINAL_SIZE_TOO_SMALL
} errors_shell;

typedef enum errors_replacement
{
	REQUESTED_SHELL_ARGUMENT_IS_OUT_OF_RANGE = 16,
	MISSING_ENVIRONMENT_VALUE,
	MISSING_REPLACEMENT_VALUE_NAME,
	REQUESTED_HISTORY_COMMAND_IS_OUT_OF_RANGE
} errors_replacement;

typedef enum errors_parser
{
	USING_SHELL_CONTROL_CHARACTERS_WITHOUT_ANY_ARGUMENTS_READ = 20,
	INCORRECT_COMMAND_CHARACTER_WAS_USED,
	MISSING_PROGRAM_BODY,
	MISSING_STDIN_FILE,
	MISSING_STDOUT_FILE,
	SECOND_INPUT_FILE,
	SECOND_OUTPUT_FILE,
	BACKGROUND_CONTROL_CHARACTER_POSITION_IS_WRONG,
	ODD_ARGUMENT_OR_INCORRECT_PLACE,
	INCORRECT_QUOTES_FORMATTING,
	NOTHING_TO_EXECUTE
} errors_parser;

typedef enum errors_executor
{
	TOO_FEW_ARGUMENTS = 31,
	TOO_MANY_ARGUMENTS,
	UNRECOGNIZED_KEY,
	EXTRA_STDIN,
	EXTRA_STDOUT,
	SECOND_ARGUMENT_IS_NOT_A_NUMBER,
	INVALID_JOB_HISTORY_NUMBER,
	FAILED_TO_CREATE_A_PIPE,
	FAILED_TO_OPEN_A_FILE,
	FAILED_TO_FORK_A_PROCESS,
	FAILED_TO_CLOSE_A_STREAM,
	FAILED_TO_REDIRECT_STREAM,
	FAILED_TO_WAIT_CHILD_STATUS,
	FAILED_TO_LAUNCH_A_PROCESS,
	FAILED_TO_CHANGE_PROCESS_GROUP,
	FAILED_TO_CHANGE_DIRECTORY,
	FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP,
	MISSING_OR_INACCESSIBLE_HELP_FILE
} errors_executor;

typedef enum errors_signals
{
	FAILED_TO_ATTACH_SIGNAL_HANDLER = 49,
	SIGINT_RECEIVED,
	A_PROCESS_BECOME_AN_ORPHAN,
	UNDEFINED_SIGNAL_RECEIVED
} errors_signals;

extern const char *error_messages[AMOUNT_OF_ERROR_MESSAGES];

#endif /* __ERROR_TYPES_H__ */