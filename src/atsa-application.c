/* atsa-application.c
 *
 * Copyright 2025 nam
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h> // Includes GtkFileDialog, GtkFileFilter, GListStore, etc.
#include "atsa-application.h"
#include "atsa-window.h"
#include "rust_questions_api.h"
struct _AtsaApplication
{
	AdwApplication parent_instance;
};

G_DEFINE_FINAL_TYPE (AtsaApplication, atsa_application, ADW_TYPE_APPLICATION)

AtsaApplication *
atsa_application_new (const char        *application_id,
                      GApplicationFlags  flags)
{
	g_return_val_if_fail (application_id != NULL, NULL);

	return g_object_new (ATSA_TYPE_APPLICATION,
	                     "application-id", application_id,
	                     "flags", flags,
	                     "resource-base-path", "/org/nam/atsa",
	                     NULL);
}

static void
atsa_application_activate (GApplication *app)
{
	GtkWindow *window;

	g_assert (ATSA_IS_APPLICATION (app));

	window = gtk_application_get_active_window (GTK_APPLICATION (app));

	if (window == NULL)
		window = g_object_new (ATSA_TYPE_WINDOW,
		                       "application", app,
		                       NULL);

	gtk_window_present (window);
}

static void
atsa_application_class_init (AtsaApplicationClass *klass)
{
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	app_class->activate = atsa_application_activate;
}

// --- Callback for Open Folder (GtkFileDialog) completion ---
// This function is called once the user has made a selection or cancelled the folder dialog.
static void
open_folder_dialog_response_cb (GObject      *source_object,
                                GAsyncResult *res,
                                gpointer      user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
    GFile         *folder = NULL;
    GError        *error = NULL;

    // Finish the asynchronous folder selection operation
    folder = gtk_file_dialog_select_folder_finish (dialog, res, &error);

    if (error)
    {
        // Handle any errors during folder selection
        g_printerr ("Failed to select folder: %s\n", error->message);
        g_error_free (error);
    }
    else if (folder)
    {
        // If a folder was selected, get its path and print it
        gchar *path = g_file_get_path (folder);
        g_print ("Selected folder: %s\n", path);
        g_free (path);
        g_object_unref (folder); // Release the GFile object
        // TODO: Add your main logic here to open the selected project folder.
        // This could involve loading its contents, switching to a new view, etc.
    }
    else
    {
        // User cancelled the dialog
        g_print ("Folder selection cancelled.\n");
    }

    // Always unref the dialog object after its use is complete
    g_object_unref (dialog);
}

// --- Application-level action for "Open Project" (Open Folder) ---
// This function is triggered when the "Open Folder" button is clicked.
static void
atsa_application_open_project_action (GSimpleAction *action,
                                      GVariant      *parameter,
                                      gpointer       user_data)
{
    AtsaApplication *self = user_data;
    GtkWindow       *parent_window = NULL;
    GtkFileDialog   *dialog = NULL;

    g_assert (ATSA_IS_APPLICATION (self));

    // Get the active window to use as the parent for the dialog, ensuring proper transient behavior.
    parent_window = gtk_application_get_active_window (GTK_APPLICATION (self));

    // Create a new GtkFileDialog instance.
    dialog = gtk_file_dialog_new ();
    gtk_file_dialog_set_title (dialog, _("Open Folder")); // Set the dialog title

    // Set the dialog to be modal and transient for the main window.
    gtk_file_dialog_set_modal (dialog, TRUE);

    // Call the asynchronous method to select a folder.
    // The 'open_folder_dialog_response_cb' will be called upon completion.
    gtk_file_dialog_select_folder (dialog,
                                   parent_window, // Transient parent (can be NULL)
                                   NULL,          // GCancellable (can be NULL for no cancellation support)
                                   open_folder_dialog_response_cb, // Callback function for response
                                   NULL);         // User data to pass to the callback (not used here)
}


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


static void
atsa_application_open_file_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
    AtsaApplication *self = user_data;
    GtkWindow       *parent_window = NULL;
    GtkFileDialog   *dialog = NULL;
    GListStore      *filters = NULL;
    GtkFileFilter   *yaml_filter = NULL;
    GtkFileFilter   *all_files_filter = NULL;

    g_assert (ATSA_IS_APPLICATION (self));

    parent_window = gtk_application_get_active_window (GTK_APPLICATION (self));

    dialog = gtk_file_dialog_new ();
    gtk_file_dialog_set_title (dialog, _("Open YAML File"));

    gtk_file_dialog_set_modal (dialog, TRUE);

    yaml_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (yaml_filter, _("YAML Files"));
    gtk_file_filter_add_pattern (yaml_filter,    "*.yaml");
    gtk_file_filter_add_pattern (yaml_filter, "*.yml");
    gtk_file_filter_add_mime_type (yaml_filter, "text/yaml");
    gtk_file_filter_add_mime_type (yaml_filter, "application/x-yaml");

    all_files_filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (all_files_filter, _("All Files"));
    gtk_file_filter_add_pattern (all_files_filter, "*");

    filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
    g_list_store_append (filters, yaml_filter);
    g_list_store_append (filters, all_files_filter);

    gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));

    g_object_unref (yaml_filter);
    g_object_unref (all_files_filter);
    g_object_unref (filters);

    gtk_file_dialog_open (dialog,
                          parent_window,
                          NULL,
                          open_file_dialog_response_cb,
                          NULL);
}

static void
atsa_application_about_action (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	static const char *developers[] = {"nam", NULL};
	AtsaApplication *self = user_data;
	GtkWindow *window = NULL;

	g_assert (ATSA_IS_APPLICATION (self));

	window = gtk_application_get_active_window (GTK_APPLICATION (self));

	adw_show_about_dialog (GTK_WIDGET (window),
	                       "application-name", "atsa",
	                       "application-icon", "org.nam.atsa",
	                       "developer-name", "nam",
	                       "translator-credits", _("translator-credits"),
	                       "version", "0.1.0",
	                       "developers", developers,
	                       "copyright", "© 2025 nam",
	                       NULL);
}

static void
atsa_application_quit_action (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
	AtsaApplication *self = user_data;

	g_assert (ATSA_IS_APPLICATION (self));

	g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
	{ "quit", atsa_application_quit_action },
	{ "about", atsa_application_about_action },
        { "open-project", atsa_application_open_project_action }, // Action for "Open Folder"
        { "open-file", atsa_application_open_file_action },
};

static void
atsa_application_init (AtsaApplication *self)
{
	g_action_map_add_action_entries (G_ACTION_MAP (self),
	                                 app_actions,
	                                 G_N_ELEMENTS (app_actions),
	                                 self);
	gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                       "app.quit",
	                                       (const char *[]) { "<primary>q", NULL });
}
