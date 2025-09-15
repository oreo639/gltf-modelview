project('gltf-modelview', ['c'],
  version: '1.0.0',
  default_options: [ 'c_std=c99', 'b_sanitize=address' ],
)

cc = meson.get_compiler('c')

lm    = cc.find_library('m', required: false)
sdl2  = dependency('sdl2')
epoxy = dependency('epoxy')
cgltf = subproject('cgltf').get_variable('cgltf_dep')
stb   = subproject('stb').get_variable('stb_dep')

# Enable some sanity checks
foreach a : ['-Wshadow', '-Wsign-compare', '-Werror=implicit-function-declaration', '-Werror=implicit-int']
  if cc.has_argument(a)
    add_project_arguments(a, language : 'c')
  endif
endforeach

subdir('src')

executable(
  'gltf_modelview',
  gview_src,
  include_directories: include_directories('src'),
  dependencies: [sdl2, epoxy, cgltf, stb, lm, idep_cutils],
  install: true,
)
