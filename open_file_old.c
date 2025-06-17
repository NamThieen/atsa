static void
open_file_dialog_response_cb (GObject      *source_object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
    GFile         *file = NULL;
    GError        *error = NULL;
    gchar         *input_path = NULL;
    size_t         total_questions = 0;
    size_t         i = 0;
    QuestionTypeC  q_type;
    char          *question_text = NULL;
    size_t         mc_option_count = 0;
    char         **mc_options = NULL;
    size_t         mc_correct_answer = 0;
    size_t         tf_statement_count = 0;
    char         **tf_statements = NULL;
    unsigned char *tf_correct_answers = NULL;
    size_t         j = 0;


    file = gtk_file_dialog_open_finish (dialog, res, &error);

    if (error)
    {
        g_printerr ("Failed to open file: %s\n", error->message);
        g_error_free (error);
    }
    else if (file)
    {
        input_path = g_file_get_path (file);
        g_print("Selected YAML file for processing: %s\n", input_path);

        if (load_questions_into_memory(input_path) == 0)
        {
            g_print("Questions loaded into Rust memory successfully.\n");

            total_questions = get_total_question_count();
            g_print("Total questions found: %zu\n", total_questions);

            for (i = 0; i < total_questions; ++i)
            {
                q_type = get_question_type(i);
                question_text = get_question_text(i);

                g_print("\n--- Question %zu ---\n", i + 1);
                if (question_text != NULL) {
                    g_print("Question Text: %s\n", question_text);
                    free_cstring(question_text);
                    question_text = NULL;
                } else {
                    g_print("Question Text: (Error or not found)\n");
                }


                if (q_type == QUESTION_TYPE_MULTIPLE_CHOICE)
                {
                    g_print("Type: Multiple Choice\n");
                    mc_option_count = 0;
                    mc_options = get_mc_options(i, &mc_option_count);
                    mc_correct_answer = get_mc_correct_answer(i);

                    if (mc_options != NULL && mc_option_count > 0) {
                        g_print("Options:\n");
                        for (j = 0; j < mc_option_count; ++j) {
                            g_print("  %zu: %s\n", j, mc_options[j]);
                        }
                        free_string_array(mc_options, mc_option_count);
                        mc_options = NULL;
                    }
                    g_print("Correct Answer Index: %zu\n", mc_correct_answer);
                }
                else if (q_type == QUESTION_TYPE_TRUE_FALSE)
                {
                    g_print("Type: True/False\n");
                    tf_statement_count = 0;
                    tf_statements = get_tf_statements(i, &tf_statement_count);
                    tf_correct_answers = get_tf_correct_answers(i, &tf_statement_count);

                    if (tf_statements != NULL && tf_correct_answers != NULL && tf_statement_count > 0) {
                        g_print("Statements:\n");
                        for (j = 0; j < tf_statement_count; ++j) {
                            g_print("  - \"%s\" -> %s\n", tf_statements[j], tf_correct_answers[j] ? "TRUE" : "FALSE");
                        }
                        free_string_array(tf_statements, tf_statement_count);
                        tf_statements = NULL;
                        free_bool_array(tf_correct_answers);
                        tf_correct_answers = NULL;
                    }
                }
                else
                {
                    g_print("Type: Unknown or Invalid\n");
                }
            }
        }
        else
        {
            g_printerr("Failed to load questions into Rust memory.\n");
        }

        g_free (input_path);
        g_object_unref (file);
    }
    else
    {
        g_print ("File selection cancelled.\n");
    }

    g_object_unref (dialog);
}

