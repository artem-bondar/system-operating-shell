#ifndef __MISCELLANEOUS_STUFF_H__
#define __MISCELLANEOUS_STUFF_H__

#define TITLE_SIZE 7
#define TITLE_WIDTH 30
#define LOAD_SCREEN_HEIGHT 9

#define BUFFER_SIZE 256

#define AMOUNT_OF_ENVIRONMENT_VALUES 8

#define INVALID_NUMBER -1

#include "error_types.h"

typedef enum control_character_type
{
	NON_CONTROL_CHARACTER,
	NORMAL_CONTROL_CHARACTER,
	ADDITIONAL_CONTROL_CHARACTER
} control_character_type;

typedef enum run_mode
{
	INTERACTIVE,
	INPUT_ONLY,
	OUTPUT_ONLY,
	AUTONOMOUS
} run_mode;

typedef enum stderr_mode
{
	TO_TERMINAL,
	TO_FILE
} stderr_mode;

typedef enum sed_modes
{
	NORMAL_SUBSTITUDE,
	BEFORE_ANOTHER_LINE,
	AFTER_ANOTHER_LINE
} sed_modes;

typedef enum grep_modes
{
	NORMAL,
	INVERSE
} grep_modes;

extern const char *environment_values_names[AMOUNT_OF_ENVIRONMENT_VALUES];
extern char environment_values_replacement_patterns[AMOUNT_OF_ENVIRONMENT_VALUES][BUFFER_SIZE];

void calculate_indents(unsigned short *indent, unsigned short *vertical_indent, const unsigned short width, const unsigned short height);
void insert_symbol_to_string(char symbol, char *position);
void delete_symbol_from_string(char *position);
control_character_type detect_control_character(const char character);
const int get_control_character_by_its_second_part(const char character);
errors_global convert_string_argument(char **string);
unsigned int detect_number_of_substrings_in_string(char *string, const char *substring);
errors_global replace_substring_in_string(char **string, const char *substring, const char *replacement_pattern);
errors_global replace_explicit_environment_values_in_string(char **string);
errors_global replace_home_sign_with_path(char **string, const char *path);
errors_global concatenate_line(sed_modes concatenate_mod, char **source_line, const char *line_to_add);
const int get_number_from_string(const char *string);

#endif /* __MISCELLANEOUS_STUFF_H__ */