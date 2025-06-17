#include "atsa-test-window.h"
#include <glib/gi18n.h> // For _() macro if you use translatable strings

struct _AtsaTestWindow
{
  AdwWindow parent_instance;
  gchar *yaml_file_path; // Store the path to the YAML file
};

G_DEFINE_FINAL_TYPE (AtsaTestWindow, atsa_test_window, ADW_TYPE_WINDOW)

// Constructor for AtsaTestWindow
AtsaTestWindow *
atsa_test_window_new (GtkApplication *app, const gchar *yaml_file_path)
{
  return g_object_new (ATSA_TYPE_TEST_WINDOW,
                       "application", app,
                       "title", _("Atsa Test"),
                       NULL);
}

// Private function to set the YAML file path after object creation
static void
atsa_test_window_set_yaml_file_path (AtsaTestWindow *self, const gchar *yaml_file_path)
{
  g_clear_pointer (&self->yaml_file_path, g_free); // Free old path if exists
  self->yaml_file_path = g_strdup (yaml_file_path); // Duplicate the string
  g_print ("AtsaTestWindow created. Will load questions from: %s\n", self->yaml_file_path);
  // TODO: In future steps, this is where you'll call Rust FFI to load questions
  // load_questions_into_memory(self->yaml_file_path);
}

static void
atsa_test_window_init (AtsaTestWindow *self)
{
  // Set default size (optional, can also be done in CSS or UI file)
  gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);

}

static void
atsa_test_window_class_init (AtsaTestWindowClass *klass)
{
  // No specific class members or methods for now
}

// GObject property setter for 'yaml-file-path'
static void
atsa_test_window_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  AtsaTestWindow *self = ATSA_TEST_WINDOW (object);

  switch (prop_id)
  {
    case 1: // Custom property ID for yaml_file_path (you could use a define for this)
      atsa_test_window_set_yaml_file_path (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

// GObject property getter for 'yaml-file-path'
static void
atsa_test_window_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  AtsaTestWindow *self = ATSA_TEST_WINDOW (object);

  switch (prop_id)
  {
    case 1:
      g_value_set_string (value, self->yaml_file_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

// Override dispose to free allocated memory
static void
atsa_test_window_dispose (GObject *object)
{
  AtsaTestWindow *self = ATSA_TEST_WINDOW (object);
  g_clear_pointer (&self->yaml_file_path, g_free);
  G_OBJECT_CLASS (atsa_test_window_parent_class)->dispose (object);
}

// Override finalize to clean up resources (should usually be after dispose, but g_clear_pointer handles this)
static void
atsa_test_window_finalize (GObject *object)
{
  // No additional finalization needed since dispose handles g_free
  G_OBJECT_CLASS (atsa_test_window_parent_class)->finalize (object);
}

// --- GObject Properties Registration ---
enum {
  PROP_0,
  PROP_YAML_FILE_PATH // Property ID for yaml_file_path
};

static void
atsa_test_window_class_init_properties (AtsaTestWindowClass *klass)
{
  g_object_class_install_property (G_OBJECT_CLASS (klass),
                                   PROP_YAML_FILE_PATH,
                                   g_param_spec_string ("yaml-file-path",
                                                        "YAML File Path",
                                                        "Path to the YAML file containing questions.",
                                                        NULL, // default value
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

// Connect set_property and get_property
void atsa_test_window_register_properties(AtsaTestWindowClass *klass) {
    G_OBJECT_CLASS(klass)->set_property = atsa_test_window_set_property;
    G_OBJECT_CLASS(klass)->get_property = atsa_test_window_get_property;
    atsa_test_window_class_init_properties(klass);
}


