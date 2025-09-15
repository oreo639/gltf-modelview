sb_test = executable(
	'sb',
	files(
		'sb.c',
	),
	include_directories: [inc_utils],
	dependencies: [idep_cutils],
)

ht_test = executable(
	'ht',
	files(
		'ht.c',
	),
	include_directories: [inc_utils],
	dependencies: [idep_cutils],
)

test('Test string buffer utility functions', sb_test)
test('Test hash table utility functions', ht_test)
