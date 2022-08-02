project(
  'ssusb',
  'c',
  version: run_command('cat', 'VERSION', check: true).stdout().strip(),
  meson_version: '>=0.57.0'
)

pkgconfig = import('pkgconfig')

project_description = 'Library for accessing Sega Saturn related USB devices'

project_headers = [
  'include/ssusb/ssusb.h',
  'include/ssusb/ssusb-export.h',
  'include/ssusb/ssusb-types.h'
]

project_source_files = [
  'drivers/crc.c',
  'drivers/datalink.c',
  'drivers/ring_buffer.c',
  'drivers/usb-cartridge.c',
  'drivers/ftdi_ext.c',
  'file.c',
  'ssusb.c',
  'ssusb_interface.c'
]

project_dependencies = [
  dependency('libftdi1', version: '>=1.5', required: true)
]

include_directories = [
  include_directories('.'),
  include_directories('include')
]

public_headers = include_directories('include')

build_args = [
  '-DEXPORT_DLL'
]

link_args = [
]

if get_option('buildtype').startswith('debug')
  project_source_files += 'debug.c'

  add_project_arguments('-DDEBUG', language: 'c')
endif

# Target

project_target = shared_library(
  meson.project_name(),
  project_source_files,
  install: true,
  c_args: build_args,
  link_args: link_args,
  gnu_symbol_visibility: 'hidden',
  include_directories: include_directories,
  dependencies: project_dependencies,
)

install_headers(
  project_headers,
  subdir: 'ssusb'
)

# Project

# Make this library usable as a Meson subproject
libssusb_dep = declare_dependency(
  include_directories: public_headers,
  link_with: project_target,
)

pkgconfig.generate(
  project_target,
  name: 'libssusb',
  subdirs: [
    '.'
  ],
  description: project_description,
  requires: project_dependencies,
)
