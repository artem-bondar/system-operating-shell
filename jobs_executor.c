#define _POSIX_SOURCE /* kill() macro requirement */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "interface_controller.h"
#include "shell_controller.h"
#include "stream_handler.h"

errors_global initialize_history(jobs_history **history_to_create)
{
	*history_to_create = (jobs_history*)malloc(sizeof(jobs_history));
	if (*history_to_create == NULL)
		return ALLOCATION_ERROR;
	(*history_to_create)->newest_record = (*history_to_create)->oldest_record = (*history_to_create)->current_record = NULL;
	(*history_to_create)->holding_records_amount = 0;
	return NO_ERROR;
}

errors_global initialize_history_record(history_record **record_to_create)
{
	*record_to_create = (history_record*)malloc(sizeof(history_record));
	if (*record_to_create == NULL)
		return ALLOCATION_ERROR;
	(*record_to_create)->jobs = NULL;
	(*record_to_create)->number_of_jobs = 0;
	(*record_to_create)->record_full_text = NULL;
	(*record_to_create)->older_record = (*record_to_create)->newer_record = NULL;
	return NO_ERROR;
}

errors_global initialize_job(job **job_to_create)
{
	*job_to_create = (job*)malloc(sizeof(job));
	if (*job_to_create == NULL)
		return ALLOCATION_ERROR;
	(*job_to_create)->execute_as = FOREGROUND;
	(*job_to_create)->programs = NULL;
	(*job_to_create)->number_of_programs = 0;
	return NO_ERROR;
}

errors_global initialize_program(program **program_to_create)
{
	*program_to_create = (program*)malloc(sizeof(program));
	if (*program_to_create == NULL)
		return ALLOCATION_ERROR;
	(*program_to_create)->std_in = (*program_to_create)->std_out = NULL;
	(*program_to_create)->arguments = NULL;
	(*program_to_create)->number_of_arguments = 0;
	(*program_to_create)->output_type = REWRITE;
	return NO_ERROR;
}

errors_global initialize_cartulary(jobs_cartulary **cartulary_to_create)
{
	*cartulary_to_create = (jobs_cartulary*)malloc(sizeof(jobs_cartulary));
	if (*cartulary_to_create == NULL)
		return ALLOCATION_ERROR;
	(*cartulary_to_create)->oldest_record = (*cartulary_to_create)->newest_record = NULL;
	(*cartulary_to_create)->holding_records_amount = (*cartulary_to_create)->jobs_numeration_counter = 0;
	return NO_ERROR;
}

errors_global initialize_cartulary_record(cartulary_record **record_to_create)
{
	*record_to_create = (cartulary_record*)malloc(sizeof(cartulary_record));
	if (*record_to_create == NULL)
		return ALLOCATION_ERROR;
	(*record_to_create)->job_name = NULL;
	(*record_to_create)->job_number = (*record_to_create)->number_of_processes = 0;
	(*record_to_create)->processes = NULL;
	(*record_to_create)->job_status = NOT_CREATED;
	(*record_to_create)->newer_record = (*record_to_create)->older_record = NULL;
	return NO_ERROR;
}

errors_global initialize_process(process **process_to_create)
{
	*process_to_create = (process*)malloc(sizeof(process));
	if (*process_to_create == NULL)
		return ALLOCATION_ERROR;
	(*process_to_create)->pid = 0;
	(*process_to_create)->exit_status = 0;
	(*process_to_create)->status = NOT_CREATED;
	(*process_to_create)->process_name = NULL;
	return NO_ERROR;
}

void free_program(program *program_to_free)
{
	int i = 0;
	if (program_to_free->number_of_arguments)
	{
		for (; i < program_to_free->number_of_arguments; i++)
			free(program_to_free->arguments[i]);
		free(program_to_free->arguments);
	}
	if (program_to_free->std_in != NULL)
		free(program_to_free->std_in);
	if (program_to_free->std_out != NULL)
		free(program_to_free->std_out);
	free(program_to_free);
}

void free_job(job *job_to_free)
{
	int i = 0;
	if (job_to_free->number_of_programs)
	{
		for (; i < job_to_free->number_of_programs; i++)
			free_program(job_to_free->programs[i]);
		free(job_to_free->programs);
	}
	free(job_to_free);
}

void free_history_record(history_record *record_to_free)
{
	int i = 0;
	if (record_to_free->number_of_jobs)
	{
		for (; i < record_to_free->number_of_jobs; i++)
			free_job(record_to_free->jobs[i]);
		free(record_to_free->jobs);
	}
	if (record_to_free->record_full_text != NULL)
		free(record_to_free->record_full_text);
	free(record_to_free);
}

void free_history(jobs_history *history_to_free)
{
	history_record *current_record = history_to_free->newest_record;
	if (current_record != NULL)
	{
		for (;;)
		{
			if (current_record->older_record != NULL)
			{
				current_record = current_record->older_record;
				free_history_record(current_record->newer_record);
			}
			else
			{
				free_history_record(current_record);
				break;
			}
		}
	}
	free(history_to_free);
}

void free_cartulary_record(cartulary_record *record_to_free)
{
	if (record_to_free->processes != NULL)
		free(record_to_free->processes);
	if (record_to_free->job_name != NULL)
		free(record_to_free->job_name);
	free(record_to_free);
}

void free_cartulary(jobs_cartulary *cartulary_to_free)
{
	cartulary_record *current_record = cartulary_to_free->newest_record;

	if (current_record != NULL)
	{
		for (;;)
		{
			if (current_record->older_record != NULL)
			{
				current_record = current_record->older_record;
				free_cartulary_record(current_record->newer_record);
			}
			else
			{
				free_cartulary_record(current_record);
				break;
			}
		}
	}
	free(cartulary_to_free);
}

errors_global create_new_job(history_record *record)
{
	job **jobs_array_pointer;

	jobs_array_pointer = (job**)realloc(record->jobs, ++record->number_of_jobs * sizeof(job*));
	if (jobs_array_pointer == NULL)
		return ALLOCATION_ERROR;
	record->jobs = jobs_array_pointer;
	if (initialize_job(&record->jobs[record->number_of_jobs - 1]))
		return ALLOCATION_ERROR;
	return NO_ERROR;
}

errors_global create_new_program(history_record *record)
{
	program **programs_array_pointer;

	if (record->jobs == NULL)
		if (create_new_job(record))
			return ALLOCATION_ERROR;
	programs_array_pointer = (program**)realloc(record->jobs[record->number_of_jobs - 1]->programs,
		++record->jobs[record->number_of_jobs - 1]->number_of_programs * sizeof(program*));
	if (programs_array_pointer == NULL)
		return ALLOCATION_ERROR;
	record->jobs[record->number_of_jobs - 1]->programs = programs_array_pointer;
	if (initialize_program(retrieve_program(record)))
		return ALLOCATION_ERROR;
	return NO_ERROR;
}

errors_global create_new_argument(history_record *record)
{
	char **arguments_array_pointer;

	if (record->jobs == NULL)
		if (create_new_job(record))
			return ALLOCATION_ERROR;
	if (record->jobs[record->number_of_jobs - 1]->programs == NULL)
		if (create_new_program(record))
			return ALLOCATION_ERROR;
	arguments_array_pointer = (char**)realloc((*retrieve_program(record))->arguments,
		(++(*retrieve_program(record))->number_of_arguments + 1) * sizeof(char*));
	if (arguments_array_pointer == NULL)
		return ALLOCATION_ERROR;
	arguments_array_pointer[(*retrieve_program(record))->number_of_arguments - 1] =
		arguments_array_pointer[(*retrieve_program(record))->number_of_arguments] = NULL;
	(*retrieve_program(record))->arguments = arguments_array_pointer;
	return NO_ERROR;
}


program **retrieve_program(history_record *record)
{
	return (record->jobs == NULL) ? NULL : (record->jobs[record->number_of_jobs - 1]->programs == NULL) ? NULL :
		&record->jobs[record->number_of_jobs - 1]->programs[record->jobs[record->number_of_jobs - 1]->number_of_programs - 1];
}

char **retrieve_argument(history_record *record)
{/* not save, can cause memory access violations if program or job don't exist, use on your own risk */
	return &(*retrieve_program(record))->arguments[(*retrieve_program(record))->number_of_arguments - 1];
}

history_record* retrieve_history_record(jobs_history *history, retrieve_type what_record)
{
	if (what_record == NEWER_RECORD)
	{
		if (history->current_record != NULL)
			history->current_record = history->current_record->newer_record;
	}
	if (what_record == OLDER_RECORD)
	{
		if (history->current_record == NULL)
			history->current_record = history->newest_record;
		else
		{
			if (history->current_record->older_record != NULL)
				history->current_record = history->current_record->older_record;
		}
	}
	return history->current_record;
}

char *retrieve_history_record_command_text(jobs_history *history, unsigned int record_number)
{
	history_record *current_record = history->oldest_record;

	/* record number is required to be valid */
	record_number--;
	while (record_number)
	{
		current_record = current_record->newer_record;
		record_number--;
	}
	return current_record->record_full_text;
}

char *retrieve_process_name_by_pid(jobs_cartulary *cartulary, pid_t pid)
{
	cartulary_record *current_record = cartulary->newest_record;
	int i;

	while (current_record != NULL)
	{
		for (i = 0; i < current_record->number_of_processes; i++)
			if (pid == current_record->processes[i].pid)
				return current_record->processes[i].process_name;
		current_record = current_record->older_record;
	}
	return NULL;
}

cartulary_record *retrieve_cartulary_record_by_job_number(jobs_cartulary *cartulary, int job_number)
{
	cartulary_record *current_record = cartulary->newest_record;

	while (current_record != NULL)
	{
		if (current_record->job_number == job_number)
			return current_record;
		current_record = current_record->older_record;
	}
	return NULL;
}

void save_new_history_record(jobs_history *history, history_record *new_record)
{
	check_history_for_occurrences_and_remove_if_found(history, new_record);
	if (history->holding_records_amount)
	{
		history->newest_record->newer_record = new_record;
		new_record->older_record = history->newest_record;
		history->newest_record = new_record;
		if (history->holding_records_amount != HISTORY_CAP)
			history->holding_records_amount++;
		else
		{
			history->oldest_record = history->oldest_record->newer_record;
			free_history_record(history->oldest_record->older_record);
			history->oldest_record->older_record = NULL;
		}
	}
	else
	{
		history->newest_record = history->oldest_record = new_record;
		history->holding_records_amount++;
	}
	history->current_record = NULL;
}

void save_new_cartulary_record(jobs_cartulary *cartulary, cartulary_record *new_record)
{
	if (cartulary->newest_record != NULL)
	{
		cartulary->newest_record->newer_record = new_record;
		new_record->older_record = cartulary->newest_record;
		cartulary->newest_record = new_record;
	}
	else
		cartulary->newest_record = cartulary->oldest_record = new_record;
	new_record->job_number = ++cartulary->holding_records_amount;
}

void update_process_status(jobs_cartulary *cartulary, pid_t pid, process_statuses process_status, int exit_status)
{
	cartulary_record *current_record = cartulary->newest_record;
	int i;

	while (current_record != NULL)
	{
		for (i = 0; i < current_record->number_of_processes; i++)
			if (pid == current_record->processes[i].pid)
			{
				current_record->processes[i].status = process_status;
				if (process_status == TERMINATED)
					current_record->processes[i].exit_status = exit_status;
				return;
			}
		current_record = current_record->older_record;
	}
}

void check_history_for_occurrences_and_remove_if_found(jobs_history *history, history_record *record)
{
	history_record *for_delete, *current_record = history->newest_record;
	while (current_record != NULL)
	{
		if (!strcmp(current_record->record_full_text, record->record_full_text))
		{
			if (current_record->newer_record != NULL)
				current_record->newer_record->older_record = current_record->older_record;
			else
				history->newest_record = current_record->older_record;
			if (current_record->older_record != NULL)
				current_record->older_record->newer_record = current_record->newer_record;
			else
				history->oldest_record = current_record->newer_record;
			history->holding_records_amount--;
			if (current_record->older_record != NULL)
			{
				for_delete = current_record;
				current_record = current_record->older_record;
				free_history_record(for_delete);
			}
			else
			{
				free_history_record(current_record);
				break;
			}
		}
		else
			current_record = current_record->older_record;
	}
}

void check_cartulary_for_jobs_statuses_changes(jobs_cartulary *cartulary)
{
	cartulary_record *current_record = cartulary->newest_record;
	process_statuses new_possible_status = NOT_CREATED;
	int i;

	while (current_record != NULL)
	{
		if (current_record->processes[0].status != current_record->job_status)
			new_possible_status = current_record->processes[0].status;
		for (i = 1; i < current_record->number_of_processes; i++)
		{
			if (current_record->processes[i].status != new_possible_status)
			{
				new_possible_status = NOT_CREATED;
				break;
			}
		}
		if (new_possible_status != NOT_CREATED)
		{
			current_record->job_status = new_possible_status;
			new_possible_status = NOT_CREATED;
		}
		current_record = current_record->older_record;
	}
}

void check_cartulary_for_stopped_conveyor(jobs_cartulary *cartulary)
{
	cartulary_record *current_record = cartulary->newest_record;
	int i;

	while (current_record != NULL)
	{
		if (current_record->job_status == RUNNING)
		{
			for (i = 0; i < current_record->number_of_processes; i++)
			{
				if (current_record->processes[i].status == STOPPED)
				{
					current_record->job_status = STOPPED;
					break;
				}
			}
		}
		current_record = current_record->older_record;
	}
}

void check_cartulary_for_terminated_jobs_and_remove_if_found(jobs_cartulary *cartulary)
{
	cartulary_record *for_delete, *current_record = cartulary->newest_record;

	while (current_record != NULL)
	{
		if (current_record->job_status == TERMINATED)
		{
			if (current_record->older_record != NULL)
				current_record->older_record->newer_record = current_record->newer_record;
			else
				cartulary->oldest_record = current_record->newer_record;
			if (current_record->newer_record != NULL)
				current_record->newer_record->older_record = current_record->older_record;
			else
				cartulary->newest_record = current_record->older_record;
			for_delete = current_record;
			current_record = current_record->older_record;
			free_cartulary_record(for_delete);
			continue;
		}
		current_record = current_record->older_record;
	}
}

errors_global parse_job_to_cartulary_record(cartulary_record *record, job *job_to_parse)
{
	int i = 0;

	record->job_name = (char*)malloc((strlen(job_to_parse->programs[0]->arguments[0]) + 1) * sizeof(char));
	strcpy(record->job_name, job_to_parse->programs[0]->arguments[0]);
 /* making a hard copy here because this pointer can become broken in
	case of free-behaviour not to duplicate same history records   */

	record->number_of_processes = job_to_parse->number_of_programs;
	record->execute_mode = job_to_parse->execute_as;
	record->processes = (process*)malloc(record->number_of_processes * sizeof(process));
	if (record->processes == NULL)
		return ALLOCATION_ERROR;
	for (; i < record->number_of_processes; i++)
	{
		record->processes[i].status = NOT_CREATED;
		record->processes[i].process_name = job_to_parse->programs[i]->arguments[0];
	}
	record->job_status = RUNNING;
	return NO_ERROR;
}

errors_replacement replace_implicit_values_in_string(char **string, argument_types string_type, jobs_history *history, char **shell_arguments, int amount_of_shell_arguments)
{
	unsigned int result_string_length = 0, number, pattern_length;
	char *result_string, *substring_start, *substring_end, *dollar_start, *exclamation_start, *variable_start,
		*replace_pattern, *copy_pointer, *search_start = *string, *end_of_string = strchr(*string, 0);
	replacement_types replacement_type;

	result_string = (char*)malloc(sizeof(char));
	if (result_string == NULL)
		return ALLOCATION_ERROR;
	result_string[0] = 0;

	while (search_start != end_of_string)
	{
		dollar_start = strchr(search_start, '$');
		variable_start = strstr(search_start, "${");
		exclamation_start = strchr(search_start, '!');
		if (string_type == DOUBLE_QUOTE_STRING)
		{
			if (variable_start != NULL && (variable_start <= dollar_start || dollar_start == NULL))
			{
				substring_start = variable_start;
				replacement_type = ENVIRONMENTAL_VALUE_REPLACE;
			}
			else
			{
				substring_start = dollar_start;
				replacement_type = SHELL_ARGUMENT_REPLACE;
			}
		}
		else
		{
			if (variable_start != NULL && (variable_start <= dollar_start || dollar_start == NULL) &&
				(variable_start < exclamation_start || exclamation_start == NULL))
			{
				substring_start = variable_start;
				replacement_type = ENVIRONMENTAL_VALUE_REPLACE;
			}
			else
			{
				if (dollar_start != NULL && (dollar_start < exclamation_start || exclamation_start == NULL))
				{
					substring_start = dollar_start;
					replacement_type = SHELL_ARGUMENT_REPLACE;
				}
				else
				{
					substring_start = exclamation_start;
					replacement_type = HISTORY_REPLACE;
				}
			}
		}
		if (substring_start == NULL)
		{
			if (search_start == *string)
			{
				free(result_string);
				return NO_ERROR;
			}
			result_string_length += strlen(search_start);
			copy_pointer = (char*)realloc(result_string, (result_string_length + 1) * sizeof(char));
			if (copy_pointer == NULL)
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return ALLOCATION_ERROR;
			}
			result_string = copy_pointer;
			strcat(result_string, search_start);
			break;
		}
		substring_end = substring_start;
		if (replacement_type != ENVIRONMENTAL_VALUE_REPLACE)
		{
			for (number = 0, substring_end++; ; substring_end++)
			{
				if (substring_end[0] != 0 && isdigit(substring_end[0]))
					number = number * 10 + substring_end[0] - '0';
				else
					break;
			}
			if (substring_end == substring_start + sizeof(char))
			{
				result_string_length++;
				copy_pointer = (char*)realloc(result_string, (result_string_length + 1) * sizeof(char));
				if (copy_pointer == NULL)
				{
					free(result_string);
					free(*string);
					*string = NULL;
					return ALLOCATION_ERROR;
				}
				result_string = copy_pointer;
				strncat(result_string, search_start, sizeof(char));
				search_start = substring_end;
				continue;
			}
		}
		else
		{
			for (substring_end++; substring_end[0] != 0; substring_end++)
			{
				if (substring_end[0] == '}')
					break;
			}
			if (substring_end == substring_start + sizeof(char) && substring_end[0] == '}')
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return MISSING_REPLACEMENT_VALUE_NAME;
			}
			if (substring_end[0] == 0)
			{
				result_string_length++;
				copy_pointer = (char*)realloc(result_string, (result_string_length + 1) * sizeof(char));
				if (copy_pointer == NULL)
				{
					free(result_string);
					free(*string);
					*string = NULL;
					return ALLOCATION_ERROR;
				}
				result_string = copy_pointer;
				strncat(result_string, search_start, sizeof(char));
				search_start = substring_end;
				continue;
			}
			substring_end++;
		}
		if (replacement_type == SHELL_ARGUMENT_REPLACE)
		{
			if (number > amount_of_shell_arguments)
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return REQUESTED_SHELL_ARGUMENT_IS_OUT_OF_RANGE;
			}
			replace_pattern = shell_arguments[number - 1];
		}
		if (replacement_type == HISTORY_REPLACE)
		{
			if (number > HISTORY_CAP)
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return REQUESTED_HISTORY_COMMAND_IS_OUT_OF_RANGE;
			}
			replace_pattern = retrieve_history_record_command_text(history, number);
		}
		if (replacement_type == ENVIRONMENTAL_VALUE_REPLACE)
		{
			pattern_length = strlen(substring_start) - strlen(substring_end) - 3; /* -3 not to copy ${ */
			copy_pointer = (char*)malloc((pattern_length + 1) * sizeof(char));
			if (copy_pointer == NULL)
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return ALLOCATION_ERROR;
			}
			copy_pointer[0] = 0;
			strncat(copy_pointer, substring_start + 2 * sizeof(char), pattern_length);
			replace_pattern = getenv(copy_pointer);
			free(copy_pointer);
			if (replace_pattern == NULL)
			{
				free(result_string);
				free(*string);
				*string = NULL;
				return MISSING_ENVIRONMENT_VALUE;
			}
		}
		result_string_length += strlen(replace_pattern) + strlen(search_start) - strlen(substring_start);
		copy_pointer = (char*)realloc(result_string, (result_string_length + 1) * sizeof(char));
		if (copy_pointer == NULL)
		{
			free(result_string);
			free(*string);
			*string = NULL;
			return ALLOCATION_ERROR;
		}
		result_string = copy_pointer;
		strncat(result_string, search_start, strlen(search_start) - strlen(substring_start));
		strcat(result_string, replace_pattern);
		search_start = substring_end;
	}
	free(*string);
	*string = result_string;
	return NO_ERROR;
}

errors_parser parse_record_text_to_executable_job(jobs_history *history, history_record *record, char **shell_arguments, int amount_of_shell_arguments)
{
	char character;
	char *copy_pointer = NULL;
	int i = 0, start_position, command_length = strlen(record->record_full_text);
	indicator stdin = NOT_READ, stdout = NOT_READ, background_type = NOT_READ, pipe = NOT_READ;
	argument_types argument_type = UNDEFINED;
	parsing_status status = NOTHING_READ;
	errors_replacement possible_error = NO_ERROR;

 /* first parsing stage - active parsing body of command */
	for (record->jobs = NULL, record->number_of_jobs = 0, character = record->record_full_text[0];
		i < command_length; i++, character = record->record_full_text[i])
	{
		switch (status)
		{
		case WAITING_FOR_ARGUMENTS:
		{
			if (character == '#')
			{
				i = command_length;
				break;
			}
			if (character == ' ' || character == '\t')
				continue;
			if (character == ';')
			{
				if (pipe == WAITING && !(*retrieve_program(record))->number_of_arguments)
					return MISSING_PROGRAM_BODY;
				if (stdin == WAITING)
					return MISSING_STDIN_FILE;
				if (stdout == WAITING)
					return MISSING_STDOUT_FILE;
				create_new_job(record);
				stdin = stdout = background_type = pipe = NOT_READ;
				break;
			}
			if (character == '<')
			{
				if (retrieve_program(record) == NULL || !(*retrieve_program(record))->number_of_arguments)
					return MISSING_PROGRAM_BODY;
				if (background_type == READ)
					return BACKGROUND_CONTROL_CHARACTER_POSITION_IS_WRONG;
				if (stdin == READ || stdin == WAITING)
					return SECOND_INPUT_FILE;
				if (stdout == WAITING)
					return MISSING_STDOUT_FILE;
				stdin = WAITING;
				break;
			}
			if (character == '>')
			{
				if (retrieve_program(record) == NULL || !(*retrieve_program(record))->number_of_arguments)
					return MISSING_PROGRAM_BODY;
				if (background_type == READ)
					return BACKGROUND_CONTROL_CHARACTER_POSITION_IS_WRONG;
				if (stdout == READ || stdout == WAITING)
					return SECOND_OUTPUT_FILE;
				if (stdin == WAITING)
					return MISSING_STDIN_FILE;
				stdout = WAITING;
				if (i + 1 < command_length && record->record_full_text[i + 1] == '>')
				{
					i++;
					(*retrieve_program(record))->output_type = APPEND;
				}
				break;
			}
			if (character == '|')
			{
				if (retrieve_program(record) == NULL || !(*retrieve_program(record))->number_of_arguments)
					return MISSING_PROGRAM_BODY;
				if (background_type == READ)
					return BACKGROUND_CONTROL_CHARACTER_POSITION_IS_WRONG;
				if (stdin == WAITING)
					return MISSING_STDIN_FILE;
				if (stdout == WAITING)
					return MISSING_STDOUT_FILE;
				create_new_program(record);
				stdin = stdout = NOT_READ;
				pipe = WAITING;
				break;
			}
			if (character == '&')
			{
				if (stdin == WAITING || stdout == WAITING || background_type == READ)
					return BACKGROUND_CONTROL_CHARACTER_POSITION_IS_WRONG;
				record->jobs[record->number_of_jobs - 1]->execute_as = BACKGROUND;
				background_type = READ;
				break;
			}
			if ((stdin == READ && stdout == READ) ||
				(stdin == READ && stdout != WAITING) ||
				(stdin != WAITING && stdout == READ))
				return ODD_ARGUMENT_OR_INCORRECT_PLACE;
			status = READING_ARGUMENT;
			argument_type = (character == '\'' || character == '\"') ? (character == '\'') ?
				SINGLE_QUOTE_STRING : DOUBLE_QUOTE_STRING : LITERAL;
			start_position = i;
			if (character == '\\')
			{
				if (i + 1 == command_length)
					return INCORRECT_COMMAND_CHARACTER_WAS_USED;
				i++;
				character = record->record_full_text[i];
				if (detect_control_character(character) == NON_CONTROL_CHARACTER)
					return INCORRECT_COMMAND_CHARACTER_WAS_USED;
			}
			break;
		}
		case READING_ARGUMENT:
		{
			if (argument_type == LITERAL)
			{
				if (character == '\\')
				{
					if (i + 1 == command_length)
						return INCORRECT_COMMAND_CHARACTER_WAS_USED;
					i++;
					character = record->record_full_text[i];
					if (detect_control_character(character) == NON_CONTROL_CHARACTER)
						return INCORRECT_COMMAND_CHARACTER_WAS_USED;
					continue;
				}
				if (character == ' ' || character == '\t' ||
					detect_control_character(character) == ADDITIONAL_CONTROL_CHARACTER)
				{
					copy_pointer = (char*)malloc((i - start_position + 1) * sizeof(char));
					if (copy_pointer == NULL)
						return ALLOCATION_ERROR;
					strncpy(copy_pointer, record->record_full_text + start_position * sizeof(char), i - start_position);
					copy_pointer[i - start_position] = 0;
					if (convert_string_argument(&copy_pointer))
						return ALLOCATION_ERROR;
					if (replace_explicit_environment_values_in_string(&copy_pointer))
						return ALLOCATION_ERROR;
					if ((possible_error = replace_implicit_values_in_string(&copy_pointer, LITERAL, history, shell_arguments, amount_of_shell_arguments)))
						return possible_error;
					if (detect_control_character(character) == ADDITIONAL_CONTROL_CHARACTER)
						i--;
				}
			}
			if (argument_type == SINGLE_QUOTE_STRING || argument_type == DOUBLE_QUOTE_STRING)
			{
				if (character == '\\')
				{
					if (i + 1 == command_length)
						return INCORRECT_COMMAND_CHARACTER_WAS_USED;
					i++;
					character = record->record_full_text[i];
					if (detect_control_character(character) != NORMAL_CONTROL_CHARACTER)
						return INCORRECT_COMMAND_CHARACTER_WAS_USED;
					continue;
				}
				if ((character == '\'' && argument_type == SINGLE_QUOTE_STRING) ||
					(character == '\"' && argument_type == DOUBLE_QUOTE_STRING))
				{
					if (i + 1 < command_length && record->record_full_text[i + 1] != ' ' &&
						record->record_full_text[i + 1] != '\t' && detect_control_character(record->record_full_text[i + 1]) != ADDITIONAL_CONTROL_CHARACTER)
						return INCORRECT_QUOTES_FORMATTING;
					copy_pointer = (char*)malloc((i - start_position + 2) * sizeof(char));
					if (copy_pointer == NULL)
						return ALLOCATION_ERROR;
					strncpy(copy_pointer, record->record_full_text + start_position * sizeof(char), i - start_position + 1);
					copy_pointer[i - start_position + 1] = 0;
					if (convert_string_argument(&copy_pointer))
						return ALLOCATION_ERROR;
					if (argument_type == DOUBLE_QUOTE_STRING)
					{
						if (replace_explicit_environment_values_in_string(&copy_pointer))
							return ALLOCATION_ERROR;
						if ((possible_error = replace_implicit_values_in_string(&copy_pointer, DOUBLE_QUOTE_STRING, history, shell_arguments, amount_of_shell_arguments)))
							return possible_error;
					}
				}
			}
			if (copy_pointer != NULL)
			{
				if (stdin == NOT_READ && stdout == NOT_READ)
				{
					create_new_argument(record);
					*retrieve_argument(record) = copy_pointer;
				}
				if (stdin == WAITING)
				{
					(*retrieve_program(record))->std_in = copy_pointer;
					stdin = READ;
				}
				if (stdout == WAITING)
				{
					(*retrieve_program(record))->std_out = copy_pointer;
					stdout = READ;
				}
				copy_pointer = NULL;
				status = WAITING_FOR_ARGUMENTS;
			}
			break;
		}
		case NOTHING_READ:
		{
			if (character == '#')
			{
				i = command_length;
				break;
			}
			if (character == ' ' || character == '\t' || character == ';')
				continue;
			if (detect_control_character(character) == ADDITIONAL_CONTROL_CHARACTER)
				return USING_SHELL_CONTROL_CHARACTERS_WITHOUT_ANY_ARGUMENTS_READ;
			status = READING_ARGUMENT;
			argument_type = (character == '\'' || character == '\"') ? (character == '\'') ?
				SINGLE_QUOTE_STRING : DOUBLE_QUOTE_STRING : LITERAL;
			start_position = i;
			if (character == '\\')
			{
				if (i + 1 == command_length)
					return INCORRECT_COMMAND_CHARACTER_WAS_USED;
				i++;
				character = record->record_full_text[i];
				if (detect_control_character(character) == NON_CONTROL_CHARACTER)
					return INCORRECT_COMMAND_CHARACTER_WAS_USED;
			}
			break;
		}
		}
	}

 /* second parsing stage - processing case, when end of string was reached */
	switch (status)
	{
	case WAITING_FOR_ARGUMENTS:
	{
		if (stdin == WAITING)
			return MISSING_STDIN_FILE;
		if (stdout == WAITING)
			return MISSING_STDOUT_FILE;
		if (pipe == WAITING && !(*retrieve_program(record))->number_of_arguments)
			return MISSING_PROGRAM_BODY;
		break;
	}
	case READING_ARGUMENT:
	{
		if (argument_type == LITERAL)
		{
			copy_pointer = (char*)malloc((i - start_position + 1) * sizeof(char));
			if (copy_pointer == NULL)
				return ALLOCATION_ERROR;
			strncpy(copy_pointer, record->record_full_text + start_position * sizeof(char), i - start_position);
			copy_pointer[i - start_position] = 0;
			if (convert_string_argument(&copy_pointer))
				return ALLOCATION_ERROR;
			if (replace_explicit_environment_values_in_string(&copy_pointer))
				return ALLOCATION_ERROR;
			if ((possible_error = replace_implicit_values_in_string(&copy_pointer, LITERAL, history, shell_arguments, amount_of_shell_arguments)))
				return possible_error;
			if (stdin == NOT_READ && stdout == NOT_READ)
			{
				create_new_argument(record);
				*retrieve_argument(record) = copy_pointer;
			}
			if (stdin == WAITING)
				(*retrieve_program(record))->std_in = copy_pointer;
			if (stdout == WAITING)
				(*retrieve_program(record))->std_out = copy_pointer;
		}
		if (argument_type == SINGLE_QUOTE_STRING || argument_type == DOUBLE_QUOTE_STRING)
			return INCORRECT_QUOTES_FORMATTING;
		break;
	}
	case NOTHING_READ:
		return NOTHING_TO_EXECUTE;
	}
	return NO_ERROR;
}

errors_executor cd(program *parameters, const char *home_directory)
{
	if (parameters->number_of_arguments == 1)
	{
		if (chdir(home_directory))
			return FAILED_TO_CHANGE_DIRECTORY;
	}
	else
	{
		if (replace_home_sign_with_path(&parameters->arguments[1], home_directory))
			return ALLOCATION_ERROR;
		if (chdir(parameters->arguments[1]))
			return FAILED_TO_CHANGE_DIRECTORY;
	}
	return NO_ERROR;
}

errors_executor pwd(const char *current_directory)
{
	printf("%s\n", current_directory);
	return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
}

errors_executor jobs(program *parameters, jobs_cartulary *cartulary)
{
	cartulary_record *current_record = cartulary->oldest_record;

	while (current_record != NULL)
	{
		printf("[%d] %s in %s: %s\n", current_record->job_number, (current_record->job_status == RUNNING) ? "running" : "stopped",
			(current_record->execute_mode == FOREGROUND) ? "fg" : "bg", current_record->job_name);
		current_record = current_record->newer_record;
	}
	return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
}

errors_executor fg(program *parameters, jobs_cartulary *cartulary, unsigned int *amount_of_alive_processes, run_mode mode)
{
	cartulary_record *current_record = cartulary->oldest_record;
	int i = 0;

	if (parameters->number_of_arguments == 1)
	{
		while (current_record != NULL)
		{
			if (current_record->execute_mode == FOREGROUND)
				printf("[%d] %s: %s\n", current_record->job_number, (current_record->job_status == RUNNING) ? "running" : "stopped", current_record->job_name);
			current_record = current_record->newer_record;
		}
		return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
	}
	else
	{
		current_record = retrieve_cartulary_record_by_job_number(cartulary, get_number_from_string(parameters->arguments[1]));
		if (current_record->execute_mode != FOREGROUND || current_record->job_status != RUNNING)
		{
			if (current_record->execute_mode == BACKGROUND)
				current_record->execute_mode = FOREGROUND;
			for (; i < current_record->number_of_processes; i++)
			{
				if (mode != AUTONOMOUS)
				{
					if (mode != OUTPUT_ONLY && tcsetpgrp(0, current_record->processes[i].pid))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
					if (mode != INPUT_ONLY && tcsetpgrp(1, current_record->processes[i].pid))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
				}
				kill(current_record->processes[i].pid, SIGCONT);
				if ((waitpid(current_record->processes[i].pid, &current_record->processes[i].exit_status, WUNTRACED)) == -1)
					return FAILED_TO_WAIT_CHILD_STATUS;
				if (mode != AUTONOMOUS)
				{
					if (mode != OUTPUT_ONLY && tcsetpgrp(0, getpid()))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
					if (mode != INPUT_ONLY && tcsetpgrp(1, getpid()))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
				}
				current_record->processes[i].status = (WIFSTOPPED(current_record->processes[i].exit_status)) ? STOPPED : TERMINATED;
				if (current_record->processes[i].status == TERMINATED)
				{
					memset(environment_values_replacement_patterns[LAST_FINISHED_PROCESS_STATUS], 0, BUFFER_SIZE);
					sprintf(environment_values_replacement_patterns[LAST_FINISHED_PROCESS_STATUS], "%d", WEXITSTATUS(current_record->processes[i].status));
					(*amount_of_alive_processes)--;
				}
			}
		}
	}
	return NO_ERROR;
}

errors_executor bg(program *parameters, jobs_cartulary *cartulary, run_mode mode)
{
	cartulary_record *current_record = cartulary->oldest_record;
	int i = 0;

	if (parameters->number_of_arguments == 1)
	{
		while (current_record != NULL)
		{
			if (current_record->execute_mode == BACKGROUND)
				printf("[%d] %s: %s\n", current_record->job_number, (current_record->job_status == RUNNING) ? "running" : "stopped", current_record->job_name);
			current_record = current_record->newer_record;
		}
		return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
	}
	else
	{
		current_record = retrieve_cartulary_record_by_job_number(cartulary, get_number_from_string(parameters->arguments[1]));
		if (current_record->execute_mode == BACKGROUND)
			for (; i < current_record->number_of_processes; i++)
			{
				kill(current_record->processes[i].pid, (current_record->job_status == RUNNING) ? SIGTSTP : SIGCONT);
				if (current_record->processes[i].status == STOPPED)
					current_record->processes[i].status = RUNNING;
			}
		else
			current_record->execute_mode = BACKGROUND;
		return NO_ERROR;
	}
}

errors_executor show_history(jobs_history *history)
{
	int i = 0;
	history_record *current_record = history->oldest_record;

	for (; current_record != NULL; i++, current_record = current_record->newer_record)
		printf("[%02d] %s\n", i + 1, current_record->record_full_text);
	return ferror(stdout) ? FAILED_TO_WRITE_TO_STDOUT : NO_ERROR;
}

errors_executor cat()
{
	char character;

	do
	{
		character = getchar();
		if (ferror(stdin))
			return FAILED_TO_READ_FROM_STDIN;
		putchar(character);
		if (ferror(stdout))
			return FAILED_TO_WRITE_TO_STDOUT;
	} while (character != EOF);
	return NO_ERROR;
}

errors_executor sed(const char *substring, const char *replacement_pattern)
{
	char *read_line;
	errors_stream possible_error;
	sed_modes sed_mode = (!(strcmp(substring, "^"))) ? BEFORE_ANOTHER_LINE :
						 (!(strcmp(substring, "$"))) ? AFTER_ANOTHER_LINE : NORMAL_SUBSTITUDE;
	for (;;)
	{
		if ((possible_error = get_line_from_stdin(&read_line)))
			return possible_error;
		if (read_line == NULL)
			break;
		if (sed_mode == NORMAL_SUBSTITUDE)
		{
			if (replace_substring_in_string(&read_line, substring, replacement_pattern))
				return ALLOCATION_ERROR;
		}
		else
			if (concatenate_line(sed_mode, &read_line, replacement_pattern))
				return ALLOCATION_ERROR;
		printf("%s", read_line);
		if (ferror(stdout))
			return FAILED_TO_WRITE_TO_STDOUT;
		free(read_line);
	}
	return NO_ERROR;
}

errors_executor grep(grep_modes grep_mode, const char *substring)
{
	char *read_line;
	errors_stream possible_error;

	for (;;)
	{
		if ((possible_error = get_line_from_stdin(&read_line)))
			return possible_error;
		if (read_line == NULL)
			break;
		if (detect_number_of_substrings_in_string(read_line, substring))
		{
			if (grep_mode == NORMAL)
			{
				printf("%s", read_line);
				if (ferror(stdout))
					return FAILED_TO_WRITE_TO_STDOUT;
			}
		}
		else
			if (grep_mode == INVERSE)
			{
				printf("%s", read_line);
				if (ferror(stdout))
					return FAILED_TO_WRITE_TO_STDOUT;
			}
		free(read_line);
	}
	return NO_ERROR;
}

errors_executor help(run_mode mode, stderr_mode error_output)
{
	char *read_line;
	int file_descriptor;
	errors_stream possible_error;

	if ((file_descriptor = open("help.txt", O_RDONLY)) == -1)
		send_error_message_and_terminate_if_critical(MISSING_OR_INACCESSIBLE_HELP_FILE, mode, error_output);
	if (dup2(file_descriptor, 0) == -1)
		return FAILED_TO_REDIRECT_STREAM;
	if (close(file_descriptor))
		return FAILED_TO_CLOSE_A_STREAM;
	for (;;)
	{
		if ((possible_error = get_line_from_stdin(&read_line)))
			return possible_error;
		if (read_line == NULL)
			break;
		printf("%s", read_line);
		if (ferror(stdout))
			return FAILED_TO_WRITE_TO_STDOUT;
		free(read_line);
	}
	return NO_ERROR;
}

errors_executor detect_process_type_and_check_requirements_if_built_in(program *parameters, process_types *process_type)
{
	*process_type = UNDETECTED;
	if (!strcmp(parameters->arguments[0], built_in_commands_names[CD]))
	{
		*process_type = CD;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->std_out != NULL)
			return EXTRA_STDOUT;
		if (parameters->number_of_arguments > 2)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[PWD]))
	{
		*process_type = PWD;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->number_of_arguments > 1)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[JOBS]))
	{
		*process_type = JOBS;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->number_of_arguments > 1)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[FG]))
	{
		*process_type = FG;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->std_out != NULL && parameters->number_of_arguments == 2)
			return EXTRA_STDOUT;
		if (parameters->number_of_arguments > 2)
			return TOO_MANY_ARGUMENTS;
		if (parameters->number_of_arguments == 2)
		{
			if ((get_number_from_string(parameters->arguments[1]) == INVALID_NUMBER))
				return SECOND_ARGUMENT_IS_NOT_A_NUMBER;
			if ((retrieve_cartulary_record_by_job_number(cartulary, get_number_from_string(parameters->arguments[1]))) == NULL)
				return INVALID_JOB_HISTORY_NUMBER;
		}
		
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[BG]))
	{
		*process_type = BG;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->std_out != NULL && parameters->number_of_arguments == 2)
			return EXTRA_STDOUT;
		if (parameters->number_of_arguments > 2)
			return TOO_MANY_ARGUMENTS;
		if (parameters->number_of_arguments == 2)
		{
			if ((get_number_from_string(parameters->arguments[1]) == INVALID_NUMBER))
				return SECOND_ARGUMENT_IS_NOT_A_NUMBER;
			if ((retrieve_cartulary_record_by_job_number(cartulary, get_number_from_string(parameters->arguments[1]))) == NULL)
				return INVALID_JOB_HISTORY_NUMBER;
		}
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[EXIT]))
	{
		*process_type = EXIT;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->std_out != NULL)
			return EXTRA_STDOUT;
		if (parameters->number_of_arguments > 1)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[HISTORY]))
	{
		*process_type = HISTORY;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->number_of_arguments > 1)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[CAT]))
	{
		*process_type = CAT;
		if (parameters->number_of_arguments > 2)
			return TOO_MANY_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[SED]))
	{
		*process_type = SED;
		if (parameters->number_of_arguments > 3)
			return TOO_MANY_ARGUMENTS;
		if (parameters->number_of_arguments < 3)
			return TOO_FEW_ARGUMENTS;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[GREP]))
	{
		*process_type = GREP;
		if (parameters->number_of_arguments > 3)
			return TOO_MANY_ARGUMENTS;
		if (parameters->number_of_arguments == 3 && strcmp(parameters->arguments[2], "-v"))
			return UNRECOGNIZED_KEY;
	}
	if (!strcmp(parameters->arguments[0], built_in_commands_names[HELP]))
	{
		*process_type = HELP;
		if (parameters->std_in != NULL)
			return EXTRA_STDIN;
		if (parameters->number_of_arguments > 1)
			return TOO_MANY_ARGUMENTS;
	}
	if (*process_type == UNDETECTED)
		*process_type = EXTERNAL;
	return NO_ERROR;
}

errors_executor execute_command(history_record *record, unsigned int *amount_of_alive_processes, exit_statuses *exit_enquiry, run_mode mode, stderr_mode error_output)
{
	int i = 0;
	errors_executor possible_error;

	for (; i < record->number_of_jobs; i++)
	{
		if (record->jobs[i]->number_of_programs)
		{
			if ((possible_error = execute_job(record->jobs[i], amount_of_alive_processes, exit_enquiry, mode, error_output)))
				return possible_error;
		}
	}
	return NO_ERROR;
}

errors_executor execute_job(job *job_to_execute, unsigned int *amount_of_alive_processes, exit_statuses *exit_enquiry, run_mode mode, stderr_mode error_output)
{
	int i = 0, old_pipe_fds[2] = { 0 }, new_pipe_fds[2] = { 0 };
	int stdin_descriptor = -1, stdout_descriptor = -1;
	pid_t pid;
	execute_amounts executing = (job_to_execute->number_of_programs == 1) ? SINGLE_PROGRAM : CONVEYOR;
	cartulary_record *new_record;
	process_types process_type;
	errors_executor possible_error, program_result;
	indicator std_in, std_out;

	initialize_cartulary_record(&new_record);
	parse_job_to_cartulary_record(new_record, job_to_execute);
	save_new_cartulary_record(cartulary, new_record);
	for (; i < job_to_execute->number_of_programs; i++)
	{
		if ((possible_error = detect_process_type_and_check_requirements_if_built_in(job_to_execute->programs[i], &process_type)))
			send_built_in_programs_error(built_in_commands_names[process_type], possible_error, mode, error_output);
		if (!possible_error)
		{
			switch (process_type)
			{
			case CD:
			{
				if ((program_result = cd(job_to_execute->programs[i], environment_values_replacement_patterns[HOME_DIRECTORY_PATH])))
					return program_result;
				break;
			}
			case FG:
			{
				if (job_to_execute->programs[i]->number_of_arguments == 1)
					break;
				if ((program_result = fg(job_to_execute->programs[i], cartulary, amount_of_alive_processes, mode)))
					return program_result;
				break;
			}
			case BG:
			{
				if (job_to_execute->programs[i]->number_of_arguments == 1)
					break;
				if ((program_result = bg(job_to_execute->programs[i], cartulary, mode)))
					return program_result;
				break;
			}
			case EXIT:
			{
				*exit_enquiry = RECEIVED;
				break;
			}
			default:
				break;
			}
		}
		std_in = (job_to_execute->programs[i]->std_in != NULL) ? READ : NOT_READ;
		std_out = (job_to_execute->programs[i]->std_out != NULL) ? READ : NOT_READ;
		if (std_in == READ)
			if ((stdin_descriptor = open(job_to_execute->programs[i]->std_in, O_RDONLY)) == -1)
				return FAILED_TO_OPEN_A_FILE;
		if (std_out == READ)
		{
			if (job_to_execute->programs[i]->output_type == REWRITE)
			{
				if ((stdout_descriptor = open(job_to_execute->programs[i]->std_out, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
					return FAILED_TO_OPEN_A_FILE;
			}
			else
				if ((stdout_descriptor = open(job_to_execute->programs[i]->std_out, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU)) == -1)
					return FAILED_TO_OPEN_A_FILE;
		}
		if (executing == CONVEYOR)
		{
			old_pipe_fds[0] = new_pipe_fds[0];
			old_pipe_fds[1] = new_pipe_fds[1];
			if (i != job_to_execute->number_of_programs - 1)
				if (pipe(new_pipe_fds))
					return FAILED_TO_CREATE_A_PIPE;
		}
		switch ((pid = fork()))
		{
		case -1:
			return FAILED_TO_FORK_A_PROCESS;
		case 0:
		{
			if ((signal(SIGINT, universal_signal_handler)) == SIG_ERR)
				exit_with_free(FAILED_TO_ATTACH_SIGNAL_HANDLER);
			if ((signal(SIGHUP, universal_signal_handler)) == SIG_ERR)
				exit_with_free(FAILED_TO_ATTACH_SIGNAL_HANDLER);
			if ((signal(SIGTSTP, SIG_DFL)) == SIG_ERR)
				exit_with_free(FAILED_TO_ATTACH_SIGNAL_HANDLER);
			if ((signal(SIGTTOU, SIG_DFL)) == SIG_ERR)
				send_error_message_and_terminate_if_critical(FAILED_TO_ATTACH_SIGNAL_HANDLER, mode, error_output);

			if (std_in == READ)
			{
				if (dup2(stdin_descriptor, 0) == -1)
					exit_with_free(FAILED_TO_REDIRECT_STREAM);
				if (close(stdin_descriptor))
					exit_with_free(FAILED_TO_CLOSE_A_STREAM);
			}
			if (std_out == READ)
			{
				if (dup2(stdout_descriptor, 1) == -1)
					exit_with_free(FAILED_TO_REDIRECT_STREAM);
				if (close(stdout_descriptor))
					exit_with_free(FAILED_TO_CLOSE_A_STREAM);
			}
			if (executing == CONVEYOR)
			{
				if (i != job_to_execute->number_of_programs - 1)
				{
					if (close(new_pipe_fds[0]))
						exit_with_free(FAILED_TO_CLOSE_A_STREAM);
				 /* closing of inherited from parent unnecessary
					opened stdin file descriptor for next process */

					if (std_out != READ)
					{
						if (dup2(new_pipe_fds[1], 1) == -1)
							exit_with_free(FAILED_TO_REDIRECT_STREAM);
					}
					else
						if (close(new_pipe_fds[1]))
							exit_with_free(FAILED_TO_CLOSE_A_STREAM);
				}
				if (i)
				{
					if (std_in != READ)
						if (dup2(old_pipe_fds[0], 0) == -1)
							exit_with_free(FAILED_TO_REDIRECT_STREAM);
				}
			}
			pid = getpid();
			if ((setpgid(pid, pid)) == -1)
				exit_with_free(FAILED_TO_CHANGE_PROCESS_GROUP);
		 /* making every process on its own group to make sensable
			performing bg command on foreground process */

			if (process_type == EXTERNAL)
			{
				if ((execvp(job_to_execute->programs[i]->arguments[0], job_to_execute->programs[i]->arguments)) == -1)
					exit_with_free(FAILED_TO_LAUNCH_A_PROCESS);
			}
			else
			{
				if (possible_error)
					exit_with_free(possible_error);
				switch (process_type)
				{
				case PWD:
				{
					program_result = pwd(environment_values_replacement_patterns[CURRENT_DIRECTORY_PATH]);
					break;
				}
				case JOBS:
				{
					program_result = jobs(job_to_execute->programs[i], cartulary);
					break;
				}
				case FG:
				{
					program_result = (job_to_execute->programs[i]->number_of_arguments == 1) ?
								   fg(job_to_execute->programs[i], cartulary, amount_of_alive_processes, mode) : NO_ERROR;
					break;
				}
				case BG:
				{
					program_result = (job_to_execute->programs[i]->number_of_arguments == 1) ?
								   bg(job_to_execute->programs[i], cartulary, mode) : NO_ERROR;
					break;
				}
				case HISTORY:
				{
					program_result = show_history(history);
					break;
				}
				case CAT:
				{
					program_result = cat();
					break;
				}
				case SED:
				{
					program_result = sed(job_to_execute->programs[i]->arguments[1], job_to_execute->programs[i]->arguments[2]);
					break;
				}
				case GREP:
				{
					program_result = grep((job_to_execute->programs[i]->number_of_arguments == 2) ? NORMAL : INVERSE, job_to_execute->programs[i]->arguments[1]);
					break;
				}
				case HELP:
				{
					program_result = help(mode, error_output);
					break;
				}
				default:
					program_result = NO_ERROR;
				}
				exit_with_free(program_result);
			}
			break;
		}
		default:
		{
			(*amount_of_alive_processes)++;
			new_record->processes[i].pid = pid;
			new_record->processes[i].status = RUNNING;
			if (std_in == READ)
				if (close(stdin_descriptor))
					return FAILED_TO_CLOSE_A_STREAM;
			if (std_out == READ)
				if (close(stdout_descriptor))
					return FAILED_TO_CLOSE_A_STREAM;
			if (executing == CONVEYOR)
			{
			 /* closing of file descriptors that were grabbed
				by last child, leaving opened file descriptors
				for next process, that will be launched		*/
				if (i != job_to_execute->number_of_programs - 1)
					if (close(new_pipe_fds[1]))
						return FAILED_TO_CLOSE_A_STREAM;
				if (i)
					if (close(old_pipe_fds[0]))
						return FAILED_TO_CLOSE_A_STREAM;
			}
			if (job_to_execute->execute_as == FOREGROUND)
			{
				if (mode != AUTONOMOUS)
				{
					if (mode != OUTPUT_ONLY && tcsetpgrp(0, pid))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
					if (mode != INPUT_ONLY && tcsetpgrp(1, pid))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
				}
				if ((waitpid(pid, &new_record->processes[i].exit_status, WUNTRACED)) == -1)
					return FAILED_TO_WAIT_CHILD_STATUS;
				if (mode != AUTONOMOUS)
				{
					if (mode != OUTPUT_ONLY &&tcsetpgrp(0, getpid()))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
					if (mode != INPUT_ONLY && tcsetpgrp(1, getpid()))
						return FAILED_TO_CHANGE_TERMINAL_CONTROL_GROUP;
				}
				new_record->processes[i].status = (WIFSTOPPED(new_record->processes[i].exit_status)) ? STOPPED : TERMINATED;
				if (new_record->processes[i].status == TERMINATED)
				{
					(*amount_of_alive_processes)--;
					memset(environment_values_replacement_patterns[LAST_FINISHED_PROCESS_STATUS], 0, BUFFER_SIZE);
					sprintf(environment_values_replacement_patterns[LAST_FINISHED_PROCESS_STATUS], "%d", WEXITSTATUS(new_record->processes[i].exit_status));
					if (WEXITSTATUS(new_record->processes[i].exit_status) == FAILED_TO_LAUNCH_A_PROCESS)
						if (send_launch_error_message(new_record->job_name, mode, error_output))
							return FAILED_TO_WRITE_TO_STDERR;
				}
			}
		}
		}
	}
	return NO_ERROR;
}