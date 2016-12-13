#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_handler.h"
#include "shell_controller.h"

errors_stream get_shell_command_interactively(jobs_history *history)
{
	char *input_string = NULL, *copy_pointer = NULL, *cursor_position = NULL, *string_start = NULL, *string_end = NULL;
	unsigned int buffer_left = 0, buffers_used = 0, lines_used = 0;
	int first = 0, second = 0, third = 0;
	stdin_reading_status read_status = BUFFER_END;
	history_record *temporary_record = NULL, *cell_for_load;

	printf("\0337");
 /* stores cursor position */

	for (;;)
	{
		switch (read_status)
		{
		case READING:
		{
			if (buffer_left)
			{
				third = second;
				second = first;
				first = getchar();
				if (first == '\003')
				{
					free(input_string);
					putchar('\n');
					if (temporary_record != NULL)
						free(temporary_record);
					change_terminal_input_mode_to(DEFAULT);
					exit_with_free(SIGINT_RECEIVED);
				}
				if (first == '\004' || first == 26)
				{
				 /* 26 is also checked because in incanonical
					mode ^Z with flags is send in this way */
					read_status = EOF_RECEIVED;
					break;
				}
				if (first == EOF)
				{
					if (feof(stdin))
					{
						read_status = EOF_RECEIVED;
						break;
					}
					if (ferror(stdin))
					{
						free(input_string);
						if (temporary_record != NULL)
							free(temporary_record);
						return FAILED_TO_READ_FROM_STDIN;
					}
				}
				if (first == '\b' || first == 127)
				{
				 /* 127 is also checked because some
					terminals detects BackSpace as Del */
					if (buffer_left != BUFFER_SIZE && cursor_position != string_start)
					{
						printf("\033[D \033[D");
						if (cursor_position == string_end)
							input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] = 0;
						else
						{
							delete_symbol_from_string(cursor_position - sizeof(char));
							printf("%s \033[%dD", cursor_position - sizeof(char), (int)strlen(cursor_position - sizeof(char)) + 1);
						}
						buffer_left++;
						cursor_position--;
						string_end--;
					}
					break;
				}
				else
				{
					if ((putchar(first)) == EOF)
						return FAILED_TO_WRITE_TO_STDOUT;
				}
				if (third == '\033' && second == '[' &&
					(first == 'A' || first == 'B' || first == 'C' || first == 'D'))
				{
					read_status = (first == 'A') ? UP_ARROW : (first == 'B') ? DOWN_ARROW : (first == 'C') ? RIGHT_ARROW : LEFT_ARROW;
					break;
				}
				if (first != '\n')
				{
					if (cursor_position == string_end)
						input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left] = (char)first;
					else
					{
						insert_symbol_to_string(first, cursor_position);
						if (first != '\033' && (first != '[' && second != '\033'))
							printf("%s\033[%dD", cursor_position + sizeof(char), (int)strlen(cursor_position + sizeof(char)));
					}
					cursor_position++;
					string_end++;
				}
				else
				{
					if (buffer_left != BUFFER_SIZE && input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] == '\\')
					{
						printf("> ");
						input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] = 0;
						string_end--;
						string_start = cursor_position = string_end;
						first = second = third = 0;
						buffer_left++;
						lines_used++;
						break;
					}
					else
					{
						read_status = READ_FINISH;
						break;
					}
				}
				buffer_left--;
			}
			else
				read_status = BUFFER_END;
			break;
		}
		case READ_FINISH:
		case EOF_RECEIVED:
		{
			if (temporary_record != NULL)
			{
				free(temporary_record->record_full_text);
				free(temporary_record);
			}
			if (buffer_left == BUFFER_SIZE && !buffers_used)
			{
				free(input_string);
				return (read_status == EOF_RECEIVED) ? EOF_AND_EMPTY_COMMAND : EMPTY_COMMAND;
			}
			if (buffer_left != BUFFER_SIZE || buffers_used)
			{
				if (initialize_history_record(&temporary_record))
					return ALLOCATION_ERROR;
				temporary_record->record_full_text = input_string;
				save_new_history_record(history, temporary_record);
				return (read_status == EOF_RECEIVED) ? EOF_BUT_COMMAND_READ : NO_ERROR;
			}
			break;
		}
		case BUFFER_END:
		{
			if (input_string == NULL)
			{
				input_string = (char*)calloc(BUFFER_SIZE, sizeof(char));
				if (input_string == NULL)
					return ALLOCATION_ERROR;
				cursor_position = string_start = string_end = input_string;
				first = second = third = 0;
				lines_used = 0;
			}
			else
			{
				buffers_used++;
				copy_pointer = (char*)realloc(input_string, (buffers_used + 1) * BUFFER_SIZE * sizeof(char));
				if (copy_pointer == NULL)
				{
					free(input_string);
					return ALLOCATION_ERROR;
				}
				input_string = copy_pointer;
				string_end = strchr(input_string, '\0');
				string_end--;
			}
			buffer_left = BUFFER_SIZE;
			read_status = READING;
			break;
		}
		default:
	 /* UP_ARROW
		DOWN_ARROW
		LEFT_ARROW
		RIGHT_ARROW */
		{
			if (cursor_position == string_end)
			{
				input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] = 0;
				input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 2] = 0;
			}
			else
			{
				delete_symbol_from_string(cursor_position - sizeof(char));
				delete_symbol_from_string(cursor_position - 2 * sizeof(char));
			}
			buffer_left += 2;
			cursor_position -= 2;
			string_end -= 2;

			switch (read_status)
			{
			case UP_ARROW:
			case DOWN_ARROW:
			{
				printf("\0338");
				if (lines_used)
				{
					printf("\033[%dA\0337", lines_used);
					lines_used = 0;
				}
				printf("\033[J");
				if (temporary_record == NULL)
				{
					temporary_record = (history_record*)malloc(sizeof(history_record));
					if (temporary_record == NULL)
						return ALLOCATION_ERROR;
					if (buffer_left == BUFFER_SIZE && !buffers_used)
					{
						free(input_string);
						input_string = NULL;
					}
					temporary_record->record_full_text = input_string;
				}
				else
					free(input_string);
				cell_for_load = retrieve_history_record(history, (read_status == UP_ARROW) ? OLDER_RECORD : NEWER_RECORD);
				if (cell_for_load == NULL)
					cell_for_load = temporary_record;
				copy_pointer = cell_for_load->record_full_text;
				cell_for_load = NULL;
				if (copy_pointer != NULL)
				{
					buffers_used = strlen(copy_pointer);
					buffer_left = BUFFER_SIZE - buffers_used % BUFFER_SIZE;
					buffers_used /= BUFFER_SIZE;
					input_string = (char*)calloc((buffers_used + 1) * BUFFER_SIZE, sizeof(char));
					if (input_string == NULL)
						return ALLOCATION_ERROR;
					strcpy(input_string, copy_pointer);
					string_start = input_string;
					cursor_position = string_end = strchr(input_string, '\0');
					first = second = third = 0;
					printf("%s", input_string);
					read_status = READING;
				}
				else
				{
					input_string = NULL;
					read_status = BUFFER_END;
				}
				break;
			}
			case LEFT_ARROW:
			{
				if (cursor_position == string_start)
				{
					printf("\033[C");
					cursor_position++;
				}
				cursor_position--;
				read_status = READING;
				break;
			}
			case RIGHT_ARROW:
			{
				if (cursor_position == string_end)
				{
					printf("\033[D");
					cursor_position--;
				}
				cursor_position++;
				read_status = READING;
				break;
			}
			default:
			{
			 /* never will be executed something here
				just to silence that compiler warning */
				break;
			}
			}
		}
		}
	}
}

errors_stream get_shell_command_non_interactively(jobs_history *history)
{
 /* copy of previous function without printing to stdout and arrow triggers */

	char *input_string = NULL, *copy_pointer = NULL;
	unsigned int buffer_left = 0, buffers_used = 0;
	int symbol = 0;
	stdin_reading_status read_status = BUFFER_END;
	history_record *record = NULL;

	for (;;)
	{
		switch (read_status)
		{
		case READING:
		{
			if (buffer_left)
			{
				symbol = getchar();
				if (symbol == '\003')
				{
					free(input_string);
					exit_with_free(SIGINT_RECEIVED);
				}
				if (symbol == '\004')
				{
					read_status = EOF_RECEIVED;
					break;
				}
				if (symbol == EOF)
				{
					if (feof(stdin))
					{
						read_status = EOF_RECEIVED;
						break;
					}
					if (ferror(stdin))
					{
						free(input_string);
						return FAILED_TO_READ_FROM_STDIN;
					}
				}
				if (symbol != '\n')
					input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left] = (char)symbol;
				else
				{
					if (buffer_left != BUFFER_SIZE && input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] == '\\')
					{
						input_string[buffers_used * BUFFER_SIZE + BUFFER_SIZE - buffer_left - 1] = 0;
						buffer_left++;
						break;
					}
					else
					{
						read_status = READ_FINISH;
						break;
					}
				}
				buffer_left--;
			}
			else
				read_status = BUFFER_END;
			break;
		}
		case READ_FINISH:
		case EOF_RECEIVED:
		{
			if (buffer_left == BUFFER_SIZE && !buffers_used)
			{
				free(input_string);
				return (read_status == EOF_RECEIVED) ? EOF_AND_EMPTY_COMMAND : EMPTY_COMMAND;
			}
			if (buffer_left != BUFFER_SIZE || buffers_used)
			{
				if (initialize_history_record(&record))
					return ALLOCATION_ERROR;
				record->record_full_text = input_string;
				save_new_history_record(history, record);
				return (read_status == EOF_RECEIVED) ? EOF_BUT_COMMAND_READ : NO_ERROR;
			}
			break;
		}
		case BUFFER_END:
		{
			if (input_string == NULL)
			{
				input_string = (char*)calloc(BUFFER_SIZE, sizeof(char));
				if (input_string == NULL)
					return ALLOCATION_ERROR;
			}
			else
			{
				buffers_used++;
				copy_pointer = (char*)realloc(input_string, (buffers_used + 1) * BUFFER_SIZE * sizeof(char));
				if (copy_pointer == NULL)
				{
					free(input_string);
					return ALLOCATION_ERROR;
				}
				input_string = copy_pointer;
			}
			buffer_left = BUFFER_SIZE;
			read_status = READING;
			break;
		}
		default:
		{
		 /* never will be executed something here
			just to silence that compiler warning */
			break;
		}
		}
	}
}

errors_stream get_line_from_stdin(char **line)
{
	unsigned int buffers_used = 0, length;
	char *read_string, *read_result;

	for (;;)
	{
		if (!buffers_used)
		{
			read_string = (char*)calloc(1, sizeof(char) * BUFFER_SIZE);
			if (read_string == NULL)
				return ALLOCATION_ERROR;
			buffers_used++;
			read_result = fgets(read_string, BUFFER_SIZE, stdin);
			if (read_result == NULL)
			{
				if (ferror(stdin))
					return FAILED_TO_READ_FROM_STDIN;
				if (feof(stdin))
				{
					if (read_string[0] == '\0')
					{
						free(read_string);
						read_string = NULL;
					}
					break;
				}
			}
		}
		else
		{
			length = strlen(read_string);
			if (length < BUFFER_SIZE * buffers_used - 1 ||
				(length == BUFFER_SIZE * buffers_used - 1 && read_string[BUFFER_SIZE * buffers_used - 1] == '\n'))
				break;
			buffers_used++;
			read_result = (char*)realloc(read_string, sizeof(char) * BUFFER_SIZE * buffers_used);
			if (read_result == NULL)
			{
				free(read_string);
				return ALLOCATION_ERROR;
			}
			read_string = read_result;
			read_result = fgets(read_string + (BUFFER_SIZE * (buffers_used - 1)) * sizeof(char) - 1, BUFFER_SIZE + 1, stdin);
			if (read_result == NULL)
			{
				if (ferror(stdin))
					return FAILED_TO_READ_FROM_STDIN;
				if (feof(stdin))
					break;
			}
		}
	}
	*line = read_string;
	return NO_ERROR;
}