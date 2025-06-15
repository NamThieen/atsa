#ifndef RUST_QUESTIONS_API_H
#define RUST_QUESTIONS_API_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QUESTION_TYPE_NONE = 0,
    QUESTION_TYPE_MULTIPLE_CHOICE = 1,
    QUESTION_TYPE_TRUE_FALSE = 2,
} QuestionTypeC;

void free_cstring(char* s);
void free_string_array(char** array_ptr, size_t count);
void free_bool_array(unsigned char* array_ptr);

int load_questions_into_memory(const char* file_path);
size_t get_total_question_count(void);
QuestionTypeC get_question_type(size_t index);
char* get_question_text(size_t index);

char** get_mc_options(size_t index, size_t* count_out);
size_t get_mc_correct_answer(size_t index);

char** get_tf_statements(size_t index, size_t* count_out);
unsigned char* get_tf_correct_answers(size_t index, size_t* count_out);

#ifdef __cplusplus
}
#endif

#endif // RUST_QUESTIONS_API_H

