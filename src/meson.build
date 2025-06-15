atsa_sources = [
  'main.c',
  'atsa-application.c',
  'atsa-window.c',
]

incdir = include_directories('.')
# Explicitly link against the pre-built Rust library
# Meson will look for libatsa_questions.so in these paths.
# We add the current source root's lib_prebuilt directory to the linker search path.
rust_lib_dep = cc.find_library(
    'atsa_questions', # Name of the library (without 'lib' prefix or '.so' suffix)
    dirs: [meson.source_root() / 'rust_atsa_lib'], # Directory where you put the .so file
    required: true,
)
atsa_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.4'),
]

atsa_sources += gnome.compile_resources('atsa-resources',
  'atsa.gresource.xml',
  c_name: 'atsa'
)

executable('atsa', atsa_sources,
  dependencies: [atsa_deps,rust_lib_dep],
       install: true,
)
install_headers('rust_questions_api.h', install_dir : get_option('includedir') / 'org.nam.atsa')
# You might want to install the pre-built .so file if you're distributing the app
# However, for development, keeping it in lib_prebuilt is fine.
# If you want to install it with 'meson install', uncomment and adjust this:
# install_data(meson.source_root() / 'lib_prebuilt' / 'libatsa_questions.so',
#   install_dir: get_option('libdir'))