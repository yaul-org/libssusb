project(
  'ssusb',
  'c',
  version: '0.0.1'
)

project_description = 'Library for accessing Sega Saturn related USB devices'

project_headers = [
  'crc.h',
  'debug.h',
  'driver.h',
  'drivers.h',
  'fifo.h',
  'ringbuffer.h',
  'shared.h'
]

project_source_files = [
  'crc.c',
  'drivers/datalink.c',
  'drivers/usb-cartridge.c',
  'drivers.c',
  'ringbuffer.c',
  'shared.c'
]

libftdi_dep = dependency('libftdi1', version : '>=1.3', required : false)
if not libftdi_dep.found()
  libftdi_dep = dependency('libftdi', version : '>=0.20')
else
  add_project_arguments('-DHAVE_LIBFTDI1', language : 'c')
endif

project_dependencies = [
  libftdi_dep
]

build_args = [
]

# Target

public_headers = include_directories('include')

# if target_machine.system() == 'windows' or target_machine.system() == 'cygwin'
# else
# endif

project_target = static_library(
  meson.project_name(),
  project_source_files,
  install: true,
  c_args: build_args,
  gnu_symbol_visibility: 'hidden',
  include_directories: public_headers,
  dependencies: project_dependencies,
)

# Project

# Make this library usable as a Meson subproject
project_dep = declare_dependency(
  include_directories: public_headers,
  link_with: project_target,
)

set_variable('lib' + meson.project_name() + '_dep', project_dep)
