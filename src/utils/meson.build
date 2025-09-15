inc_utils = include_directories('.')

cutils_src = files(
	'strdup.c',
	'strndup.c',
	'string_buffer.c',
	'hash_table.c',
	'tests.c',
)

foreach a : ['strndup', 'strnlen', 'strdup']
	if cc.has_function(a, prefix : '#include <string.h>')
		add_project_arguments('-DHAVE_'+a.to_upper()+'=1', language : 'c')
	endif
endforeach

foreach a : ['clz']
	if cc.has_function(a)
		add_project_arguments('-DHAVE___BUILTIN_'+a.to_upper()+'=1', language : 'c')
	endif
endforeach

if cc.has_header('stdbit.h')
	add_project_arguments('-DHAVE_STDBIT_H=1', language : 'c')
endif

if cc.compiles('int x = 0; typeof(x) y;', name : 'typeof')
	add_project_arguments('-DHAVE_TYPEOF=1', language : 'c')
else
	if cc.compiles('int x = 0; __typeof__(x) y;', name : '__typeof__')
		add_project_arguments('-DHAVE___TYPEOF__=1', language : 'c')
	else
		error('Compiler support for typeof() was not detected. Try configuring with -Dc_std=c23.')
	endif
endif

libcutils = static_library('cutils',
	[cutils_src],
)

idep_cutils = declare_dependency(
	link_with: libcutils,
)

subdir('tests')
