#ifndef ATSA_TEST_WINDOW_H
#define ATSA_TEST_WINDOW_H

#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define ATSA_TYPE_TEST_WINDOW (atsa_test_window_get_type ())

G_DECLARE_FINAL_TYPE (AtsaTestWindow, atsa_test_window, ATSA, TEST_WINDOW, AdwWindow)

AtsaTestWindow *atsa_test_window_new (GtkApplication *app, const gchar *yaml_file_path);

G_END_DECLS

#endif // ATSA_TEST_WINDOW_H

