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
#include "atsa-test-window.h"
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
    AtsaApplication *app = user_data; // Get application instance to pass to new window

    file = gtk_file_dialog_open_finish (dialog, res, &error);

    if (error)
    {
        g_printerr ("Failed to open file: %s\n", error->message);
        g_error_free (error);
    }
    else if (file)
    {
        input_path = g_file_get_path (file);
        g_print("Selected YAML file: %s\n", input_path);

        // NEW: Create and show the new AtsaTestWindow
        AtsaTestWindow *test_window = atsa_test_window_new(GTK_APPLICATION(app), input_path);
        gtk_window_present(GTK_WINDOW(test_window));

        // OLD LOGIC REMOVED: Removed the direct FFI calls and printing loop here.
        // The new AtsaTestWindow will be responsible for loading and displaying questions.

        g_free (input_path); // Free the path string as it's been copied by the new window
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
