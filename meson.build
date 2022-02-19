project(
  'ssusb',
  'c',
  version: '0.0.1'
)

project_description = 'Library for accessing Sega Saturn related USB devices'

project_headers = [
  'ssusb.h'
]

project_source_files = [
  'drivers/crc.c',
  'drivers/ring_buffer.c',
  'drivers/usb-cartridge.c',
  'file.c',
  'ssusb_interface.c',
  'ssusb.c',
]

project_dependencies = [
]

include_directories = [
  include_directories('.')
]

build_args = [
]

link_args = [
]

libftdi_dep = dependency('libftdi1', version: '>=1.5', required: true)
project_dependencies += libftdi_dep

if get_option('buildtype').startswith('debug')
  project_source_files += 'debug.c'

  add_project_arguments('-DDEBUG',  language: 'c')
endif

# Target

public_headers = include_directories('include')
include_directories += public_headers

project_target = static_library(
  meson.project_name(),
  project_source_files,
  install: true,
  c_args: build_args,
  link_args: link_args,
  gnu_symbol_visibility: 'hidden',
  include_directories: include_directories,
  dependencies: project_dependencies,
)

# Project

# Make this library usable as a Meson subproject
project_dep = declare_dependency(
  include_directories: public_headers,
  link_with: project_target,
)

set_variable('lib' + meson.project_name() + '_dep', project_dep)
