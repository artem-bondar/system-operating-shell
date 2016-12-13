#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "miscellaneous_stuff.h"

const char *environment_values_names[AMOUNT_OF_ENVIRONMENT_VALUES] =
{
	"$#",
	"$?",
	"${USER}",
	"${HOME}",
	"${SHELL}",
	"${UID}",
	"${PWD}",
	"${PID}"
};

char environment_values_replacement_patterns[AMOUNT_OF_ENVIRONMENT_VALUES][BUFFER_SIZE];

void calculate_indents(unsigned short *indent, unsigned short *vertical_indent, const unsigned short width, const unsigned short height)
{
	*indent = ((width - TITLE_WIDTH) / 2) > 0 ? (width - TITLE_WIDTH) / 2 : 0;
	*vertical_indent = ((height - LOAD_SCREEN_HEIGHT) / 2) > 0 ? (height - LOAD_SCREEN_HEIGHT) / 2 : 0;
}


void insert_symbol_to_string(char symbol, char *position)
{
 /* requires at least 1 or more free malloced bytes */
	char to_save, to_replace = symbol;
	unsigned int i = 0, move_chars_amount = strlen(position);
	if (move_chars_amount)
	{
		for (; i <= move_chars_amount; i++, position++)
		{
			to_save = position[0];
			position[0] = to_replace;
			to_replace = to_save;
		}
	}
}

void delete_symbol_from_string(char *position)
{
	char to_move = position[1];
	while (to_move != '\0')
	{
		position[0] = to_move;
		position++;
		position[0] = 0;
		to_move = position[1];
	}
}

control_character_type detect_control_character(const char character)
{
	return (character == '0' || character == 'a' || character == 'b' ||
		    character == 't' || character == 'n' || character == 'v' ||
		    character == 'f' || character == 'r' || character == '\\' ||
		    character == '\'' || character == '\"') ? NORMAL_CONTROL_CHARACTER :
		   (character == '#' || character == '&' || character == '>' ||
			character == '<' || character == '|' || character == ';') ?
			ADDITIONAL_CONTROL_CHARACTER : NON_CONTROL_CHARACTER;
}

errors_global convert_string_argument(char **string)
{
	char *copy_pointer;
	unsigned int i, j, amount_of_symbols_to_omit, string_type_shift, string_length = strlen(*string);
	string_type_shift = ((*string[0]) == '\'' || (*string[0]) == '\"');
	amount_of_symbols_to_omit = string_type_shift * 2;
	for (i = string_type_shift; i < string_length - string_type_shift; i++)
		if ((*string)[i] == '\\' && i + 1 < string_length - string_type_shift)
			if (detect_control_character((*string)[i + 1]) == NORMAL_CONTROL_CHARACTER ||
			   (detect_control_character((*string)[i + 1]) == ADDITIONAL_CONTROL_CHARACTER && !string_type_shift))
			{
				amount_of_symbols_to_omit++;
				i++;
			}
	copy_pointer = (char*)malloc(sizeof(char) * (string_length - amount_of_symbols_to_omit + 1 /* for \0 */));
	if (copy_pointer == NULL)
		return ALLOCATION_ERROR;
	for (i = string_type_shift, j = 0; i < string_length - string_type_shift; i++, j++)
	{
		if ((*string)[i] == '\\' && i + 1 < string_length - string_type_shift)
			if (detect_control_character((*string)[i + 1]) == NORMAL_CONTROL_CHARACTER ||
			   (detect_control_character((*string)[i + 1]) == ADDITIONAL_CONTROL_CHARACTER && !string_type_shift))
			{
				copy_pointer[j] = get_control_character_by_its_second_part((*string)[i + 1]);
				i++;
				continue;
			}
		copy_pointer[j] = (*string)[i];
	}
	copy_pointer[j] = '\0';
	free(*string);
	*string = copy_pointer;
	return NO_ERROR;
}

const int get_control_character_by_its_second_part(const char character)
{
	switch (character)
	{
	case '0':
		return '\0';
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 't':
		return '\t';
	case 'n':
		return '\n';
	case 'v':
		return '\v';
	case 'f':
		return '\f';
	case 'r':
		return '\r';
	case '\\':
		return '\\';
	case '\'':
		return '\'';
	case '\"':
		return '\"';
	case '#':
		return '#';
	case '&':
		return '&';
	case '>':
		return '>';
	case '<':
		return '<';
	case '|':
		return '|';
	case ';':
		return ';';
	default:
		return '\0';
	}
}

unsigned int detect_number_of_substrings_in_string(char *string, const char *substring)
{
	unsigned int substrings_amount = 0;
	char *search_start = string, *end_of_string = strchr(string, '\0');;

	do
	{
		search_start = strstr(search_start, substring);
		if (search_start != NULL)
		{
			substrings_amount++;
			search_start++;
			if (search_start == end_of_string)
				break;
		}
	} while (search_start != NULL);
	return substrings_amount;
}

errors_global replace_substring_in_string(char **string, const char *substring, const char *replacement_pattern)
{
	unsigned int result_string_length, substring_length = strlen(substring), replacement_pattern_length = strlen(replacement_pattern);
	char *result_string, *end_of_string, *search_start, *substring_start;

	if ((result_string_length = detect_number_of_substrings_in_string(*string, substring)))
	{
		result_string_length *= (replacement_pattern_length - substring_length);
		result_string_length += strlen(*string);
		end_of_string = strchr(*string, '\0');
		result_string = (char*)malloc(sizeof(char) * (result_string_length + 1));
		if (result_string == NULL)
			return ALLOCATION_ERROR;
		result_string[0] = '\0';
		search_start = *string;
		while (search_start != NULL)
		{
			if (search_start == end_of_string)
				break;
			substring_start = strstr(search_start, substring);
			if (substring_start != NULL)
			{
				strncat(result_string, search_start, strlen(search_start) - strlen(substring_start));
				strcat(result_string, replacement_pattern);
			}
			else
			{
				if (search_start != *string)
					strcat(result_string, search_start);
			}
			if (substring_start != NULL)
				search_start = substring_start + sizeof(char) * substring_length;
			else
				search_start = NULL;
		}
		result_string[result_string_length] = '\0';
		free(*string);
		*string = result_string;
	}
	return NO_ERROR;
}

errors_global replace_explicit_environment_values_in_string(char **string)
{
	int i = 0;
	for (; i < AMOUNT_OF_ENVIRONMENT_VALUES; i++)
		if (replace_substring_in_string(string, (char*)environment_values_names[i], environment_values_replacement_patterns[i]))
			return ALLOCATION_ERROR;
	return NO_ERROR;
}

errors_global replace_home_sign_with_path(char **string, const char *path)
{
	char *result_string;

	if (*string[0] == '~' && (*string[1] == 0 || *string[1] == '/'))
	{
		result_string = (char *)malloc((strlen(*string) + strlen(path) - 1) * sizeof(char));
		if (result_string == NULL)
			return ALLOCATION_ERROR;
		strcpy(result_string, path);
		strcat(result_string, *string + sizeof(char));
		free(*string);
		*string = result_string;
	}
	return NO_ERROR;
}

errors_global concatenate_line(sed_modes concatenate_mod, char **source_line, const char *line_to_add)
{
	unsigned int  extended_line_length, source_line_length = strlen(*source_line), line_to_add_length = strlen(line_to_add);
	char *extended_line;

	extended_line_length = source_line_length + line_to_add_length;
	extended_line = (char*)malloc(sizeof(char) * (extended_line_length + 1));
	if (extended_line == NULL)
		return ALLOCATION_ERROR;
	if ((*source_line)[source_line_length - 1] == '\n' && concatenate_mod == AFTER_ANOTHER_LINE)
	{
		(*source_line)[source_line_length - 1] = 0;
		extended_line[extended_line_length - 1] = '\n';
		source_line_length--;
	}
	if (source_line_length > 1 && (*source_line)[source_line_length - 2] == '\r' && concatenate_mod == AFTER_ANOTHER_LINE)
	{
		(*source_line)[source_line_length - 2] = '\0';
		extended_line[extended_line_length - 2] = '\r';
		source_line_length--;
	}
	strcpy(extended_line, (concatenate_mod == BEFORE_ANOTHER_LINE) ? line_to_add : *source_line);
	strncpy((concatenate_mod == BEFORE_ANOTHER_LINE) ? extended_line + line_to_add_length * sizeof(char) : extended_line + source_line_length * sizeof(char),
			(concatenate_mod == BEFORE_ANOTHER_LINE) ? *source_line : line_to_add, (concatenate_mod == BEFORE_ANOTHER_LINE) ? source_line_length : line_to_add_length);
	extended_line[extended_line_length] = 0;
	free(*source_line);
	*source_line = extended_line;
	return NO_ERROR;
}

const int get_number_from_string(const char *string)
{
	unsigned int i = 0, number = 0, string_length = strlen(string);
	for (; i < string_length; i++)
	{
		if (isdigit(string[i]))
			number = number * 10 + string[i] - '0';
		else
			return INVALID_NUMBER;
	}
	return number;
}