// INCLUDE
#pragma once

extern char* strdup(const char*);
extern char *stpcpy(char *restrict dst, const char *restrict src);

extern char *strtok_r(char *restrict str, const char *restrict delim,
                      char **restrict saveptr);
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

// DECLARATIONS
typedef struct CSV {
    char** field_name;
    size_t num_lines;
    size_t num_fields;
    char*** lines;
} CSV;

char* file_to_buffer(const char* file_name);
void count_lines_in_file(const char* file_name, 
        size_t* num_lines, 
        size_t* longest_line_length);

void csv_free(CSV* csv);
void populate_fields(char* line, size_t line_index, CSV* csv);
void populate_field_names(const char* line, CSV* csv);
CSV* parse_csv(const char* file_name);
char* get_field(CSV* csv, size_t line, const char* name);
int get_column_index(CSV* csv, const char* name);
int search_field(CSV* csv, const char* search_name, const char* column_name);
void remove_row(CSV* csv, size_t line);
void write_csv(CSV* csv, const char* file_name);
char* csv_print(CSV* csv);
void csv_dump(CSV* csv);

#define CSV_PARSER_IMPLEMENTATION

// IMPLEMENTATIONS
#ifdef CSV_PARSER_IMPLEMENTATION

char* better_strtok(char* str, const char* del)
{
    static char* src = NULL;
    char* cur = NULL;
    char* ret = NULL;

    if (str)
    {
        src = str;
        ret = src;
    }
    if (!src)
        return NULL;
    cur = strpbrk(src, del);
    if (cur)
    {
        *cur = 0;    // set the current location to null terminator char
        ret = src;   // set the return value to the string
        src = ++cur; // increment the string.
    }
    else
    {
        src = NULL;
    }
    return ret;
}

char* file_to_buffer(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);


    char* buffer = malloc(size + 1);


    fread(buffer, 1, size, file);
    fclose(file);
    return buffer;
}

void count_lines_in_file(const char* file_name,
                             size_t* num_lines,
                             size_t* longest_line_length)
{
    *num_lines = 0;
    *longest_line_length = 0;

    FILE* file = fopen(file_name, "r");

    size_t current_length = 0;
    while(!feof(file)) {
        char ch = fgetc(file);
        if (ch == '\n') {
            *longest_line_length = 
                current_length > *longest_line_length ? 
                current_length : *longest_line_length;
            current_length = 0;
            (*num_lines)++;
        }
        else
            current_length++;
    }
}

void csv_free(CSV* csv)
{
    for (size_t i = 0; i < csv->num_lines; i++) {
        for (size_t j = 0; j < csv->num_fields; j++)
            free(csv->lines[i][j]);
        free(csv->lines[i]);
    }

    for (size_t i = 0; i < csv->num_fields; i++) {
        free(csv->field_name[i]);
    }

    free(csv->lines);
    free(csv->field_name);
    free(csv);
}

void populate_fields(char* line, size_t line_index, CSV* csv)
{
    size_t element = 0;
    assert(line_index < csv->num_lines);
    for (char* tok = better_strtok(line, ",\n"); tok; tok = better_strtok(NULL, "\n,")) {
        assert(element < csv->num_fields);
        csv->lines[line_index][element] = strdup(tok);
        element++;
    }

//     size_t element = 0;
//     for (tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
//         assert(line_index < csv->num_lines);
//         assert(element < csv->num_fields);
//         csv->lines[line_index][element] = strdup(tok);
//         element++;
//     }
}

void populate_field_names(const char* line, CSV* csv)
{
    size_t num_fields = 0;
    const char* tok;

    size_t longest_field_name = 0;

    char* copy = strdup(line);
    // get num_fields
    for (tok = strtok(copy, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
        size_t cur_tok_len = strlen(tok);

        if (cur_tok_len > longest_field_name)
            longest_field_name = cur_tok_len;

        num_fields++;
    }

    free(copy);

    csv->num_fields = num_fields;
    csv->field_name = malloc(sizeof(char*) * csv->num_fields);

    copy = strdup(line);
    // initialize fields in CSV
    size_t i = 0;
    for (tok = strtok(copy, ","); tok && *tok; tok = strtok(NULL, ",\n")) {
        char* field = strdup(tok);
        csv->field_name[i] = field;
        i++;
    }

    free(copy);
    csv->lines = malloc(csv->num_lines * sizeof(char***));

    for (size_t i = 0; i < csv->num_lines; i++) {
        csv->lines[i] = malloc(csv->num_fields * sizeof(char**));
    }
}


CSV* parse_csv(const char* file_name)
{
    size_t num_lines;
    size_t longest_line_length;

    count_lines_in_file(file_name, &num_lines, &longest_line_length);

    FILE* stream = fopen(file_name, "r");
    char line[++longest_line_length];

    CSV* csv = malloc(sizeof(CSV));
    csv->num_lines = num_lines - 1; // we dont care about the first line (col_names);
    
    size_t line_index = 0;
    while (fgets(line, 1024, stream))
    {
        if (!csv->field_name) {
            populate_field_names(line, csv);
        } else {
            populate_fields(line, line_index, csv);
            line_index++;
        }
    }
    return csv;
}

int get_column_index(CSV* csv, const char* name) {

    for (int i = 0; i < (int)csv->num_fields; i++) {
        int result = strcmp(csv->field_name[i], name);
        if (result == 0) return i;
    }

    return -1;
}

// returns line number
int search_field(CSV* csv, const char* search_name, const char* column_name)
{
    int raw_col_index = get_column_index(csv, column_name);
    if (raw_col_index == -1) 
        return -1;

    size_t col_index = (int) raw_col_index;

    for (size_t i = 0; i < csv->num_lines; i++) {
       if (strcmp(csv->lines[i][col_index], search_name) == 0)
           return (int) i;
    }
    return -1;
}

char* get_field(CSV* csv, size_t line, const char* col_name)
{
    int raw_col_index = get_column_index(csv, col_name);
    if (raw_col_index == -1) 
        return "";

    size_t col_index = (size_t) raw_col_index;
    assert(col_index < csv->num_fields);
    
    return csv->lines[line][col_index];
}

void remove_row(CSV* csv, size_t line)
{
    // Check out of bounds.
    assert(line < csv->num_lines);
    
    // last element
    bool last_element = line == csv->num_lines - 1;

    char** row_to_remove = csv->lines[line];

    // dont need to move memory if it is last element, just free.
    if (!last_element) {
        memmove(csv->lines + line, csv->lines + line + 1, sizeof(char**) * (csv->num_lines - line));
    }

    // Remove last element
     for (size_t i = 0; i < csv->num_fields; i++)
         free(row_to_remove[i]);
     free(row_to_remove);

    csv->num_lines--;
}

void csv_dump(CSV* csv)
{
        
    //print field names first
    for (size_t i = 0; i < csv->num_fields; i++) {
        printf("%s", csv->field_name[i]);
        if (i < csv->num_fields - 1)
        printf(", ");
    }
    printf("\n");

    for (size_t i = 0; i < csv->num_lines; i++) {
        for (size_t j = 0; j < csv->num_fields; j++) {
            printf("%s", csv->lines[i][j]);
            if (j < csv->num_fields - 1)
            printf(", ");
        }
        printf("\n");
    }

    printf("num lines is %zu\n", csv->num_lines);
}

// Includes commas, newlines, and null byte
size_t num_bytes_with_delim(CSV* csv)
{
    size_t total_size = 0;

    // get field names length
    for (size_t i = 0; i < csv->num_fields; i++) {
        size_t current_size = strlen(csv->field_name[i]);
        ++current_size; // Include delim or \n
        total_size += current_size; //append to total_size
    }
    
    for (size_t i = 0; i < csv->num_lines; i++) {
        for (size_t j = 0; j < csv->num_fields; j++) {
            size_t current_size = strlen(csv->lines[i][j]);
            ++current_size; // include delim, or \n, or \0
            total_size += current_size; //append to total_size
        }
    }
    return total_size;
}

char* csv_print(CSV* csv)
{
    assert (csv->num_fields > 0);
    assert (csv->num_lines > 0);

    size_t length = num_bytes_with_delim(csv);
    char* printed = malloc(sizeof(char) * length);
    char* printed_offset = printed;


    // print fields
    for (size_t i = 0; i < csv->num_fields; i++) {
        printed_offset = stpcpy(printed_offset, csv->field_name[i]);
        
        // last
        if (i == csv->num_fields - 1)
            printed_offset = stpcpy(printed_offset, "\n");
        else
            printed_offset = stpcpy(printed_offset, ",");
    }

    for (size_t i = 0; i < csv->num_lines; i++) {
        for (size_t j = 0; j < csv->num_fields; j++) {
            printed_offset = stpcpy(printed_offset, csv->lines[i][j]);
            if (j < csv->num_fields - 1)
                printed_offset = stpcpy(printed_offset, ",");
        }
        if (i < csv->num_lines - 1)
                printed_offset = stpcpy(printed_offset, "\n");
    }

    return printed;
}

void write_csv(CSV* csv, const char* file_name)
{
    FILE* output_file = fopen(file_name, "w");
    assert(output_file);

    char* csv_string = csv_print(csv);
    fputs(csv_string, output_file);

    fclose(output_file);
    free(csv_string);
}

#endif // CSV_PARSER_IMPLEMENTATION
