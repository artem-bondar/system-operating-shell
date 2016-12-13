#ifndef __JOBS_EXECUTOR_H__
#define __JOBS_EXECUTOR_H__

#include <sys/types.h>

#include "miscellaneous_stuff.h"

#define HISTORY_CAP 50

typedef enum exit_statuses
{
	NOT_RECEIVED,
	RECEIVED
} exit_statuses;

typedef enum write_type
{
	REWRITE,
	APPEND
} write_type;

typedef enum execute_type
{
	FOREGROUND,
	BACKGROUND
} execute_type;

typedef enum retrieve_type
{
	OLDER_RECORD,
	NEWER_RECORD
} retrieve_type;

typedef struct program
{
	unsigned int number_of_arguments;
	char **arguments;
	char *std_in, *std_out;
	write_type output_type;
} program;

typedef struct job
{
	execute_type execute_as;
	program **programs;
	unsigned int number_of_programs;
} job;

typedef struct history_record
{
	job **jobs;
	unsigned int number_of_jobs;
	char *record_full_text;
	struct history_record *newer_record, *older_record;
} history_record;

typedef struct jobs_history
{
	unsigned int holding_records_amount;
	history_record *newest_record, *oldest_record, *current_record;
} jobs_history;

typedef enum process_statuses
{
	NOT_CREATED,
	RUNNING,
	STOPPED,
	TERMINATED
} process_statuses;

typedef enum process_types
{
	UNDETECTED,
	EXTERNAL,
	CD,
	PWD,
	JOBS,
	FG,
	BG,
	EXIT,
	HISTORY,
	CAT,
	SED,
	GREP,
	HELP
} process_types;

typedef enum execute_amounts
{
	SINGLE_PROGRAM,
	CONVEYOR
} execute_amounts;

typedef struct process
{
	pid_t pid;
	process_statuses status;
	int exit_status;
	char *process_name;
} process;

typedef struct cartulary_record
{
	char *job_name;
	unsigned int job_number;
	execute_type execute_mode;
	process *processes;
	process_statuses job_status;
	unsigned int number_of_processes;
	struct cartulary_record *newer_record, *older_record;
} cartulary_record;

typedef struct jobs_cartulary
{
	cartulary_record *newest_record, *oldest_record;
	unsigned int holding_records_amount;
	unsigned int jobs_numeration_counter;
} jobs_cartulary;

typedef enum indicator
{
	NOT_READ,
	WAITING,
	READ
} indicator;

typedef enum argument_types
{
	UNDEFINED,
	LITERAL,
	SINGLE_QUOTE_STRING,
	DOUBLE_QUOTE_STRING
} argument_types;

typedef enum replacement_types
{
	NO_REPLACE,
	HISTORY_REPLACE,
	SHELL_ARGUMENT_REPLACE,
	ENVIRONMENTAL_VALUE_REPLACE
} replacement_types;

typedef enum parsing_status
{
	NOTHING_READ,
	WAITING_FOR_ARGUMENTS,
	READING_ARGUMENT
} parsing_status;

extern run_mode mode;
extern stderr_mode error_output;
extern jobs_cartulary *cartulary;
extern jobs_history *history;

errors_global initialize_history(jobs_history **history_to_create);
errors_global initialize_history_record(history_record **record_to_create);
errors_global initialize_job(job **job_to_create);
errors_global initialize_program(program **program_to_create);
errors_global initialize_cartulary(jobs_cartulary **cartulary_to_create);
errors_global initialize_cartulary_record(cartulary_record **record_to_create);
errors_global initialize_process(process **process_to_create);

void free_program(program *program_to_free);
void free_job(job *job_to_free);
void free_history_record(history_record *record_to_free);
void free_history(jobs_history *history_to_free);
void free_cartulary_record(cartulary_record *record_to_free);
void free_cartulary(jobs_cartulary *cartulary_to_free);

errors_global create_new_argument(history_record *record);
errors_global create_new_program(history_record *record);
errors_global create_new_job(history_record *record);

program **retrieve_program(history_record *record);
history_record *retrieve_history_record(jobs_history *history, retrieve_type what_record);
cartulary_record *retrieve_cartulary_record_by_job_number(jobs_cartulary *cartulary, int job_number);
char **retrieve_argument(history_record *record);
char *retrieve_history_record_command_text(jobs_history *history, unsigned int record_number);
char *retrieve_process_name_by_pid(jobs_cartulary *cartulary, pid_t pid);

void save_new_history_record(jobs_history *history, history_record *new_record);
void save_new_cartulary_record(jobs_cartulary *cartulary, cartulary_record *new_record);
void update_process_status(jobs_cartulary *cartulary, pid_t pid, process_statuses change_type, int exit_status);
void check_history_for_occurrences_and_remove_if_found(jobs_history *history, history_record *record);
void check_cartulary_for_jobs_statuses_changes(jobs_cartulary *cartulary);
void check_cartulary_for_stopped_conveyor(jobs_cartulary *cartulary);
void check_cartulary_for_terminated_jobs_and_remove_if_found(jobs_cartulary *cartulary);

errors_replacement replace_implicit_values_in_string(char **string, argument_types string_type, jobs_history *history, char **shell_arguments, int amount_of_shell_arguments);
errors_parser parse_record_text_to_executable_job(jobs_history *history, history_record *record, char **shell_arguments, int amount_of_shell_arguments);
errors_global parse_job_to_cartulary_record(cartulary_record *record, job *job_to_parse);

errors_executor cd(program *parameters, const char *home_directory);
errors_executor pwd(const char *current_directory);
errors_executor jobs(program *parameters, jobs_cartulary *cartulary);
errors_executor fg(program *parameters, jobs_cartulary *cartulary, unsigned int *amount_of_alive_processes, run_mode mode);
errors_executor bg(program *parameters, jobs_cartulary *cartulary, run_mode mode);
errors_executor show_history(jobs_history *history);
errors_executor cat();
errors_executor sed(const char *substring, const char *replacement_pattern);
errors_executor grep(grep_modes grep_mode, const char *substring);
errors_executor help(run_mode mode, stderr_mode error_output);

errors_executor detect_process_type_and_check_requirements_if_built_in(program *parameters, process_types *process_type);
errors_executor execute_command(history_record *record, unsigned int *amount_of_alive_processes, exit_statuses *exit_enquiry, run_mode mode, stderr_mode error_output);
errors_executor execute_job(job *job_to_execute, unsigned int *amount_of_alive_processes, exit_statuses *exit_enquiry, run_mode mode, stderr_mode error_output);

#endif /* __JOBS_EXECUTOR_H__ */