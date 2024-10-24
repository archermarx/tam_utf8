project('unicode', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c2x'])

add_project_arguments(
  '-Wno-pointer-sign', language: 'c'
)

sources = [
  'src/main.c',
  'src/utf8.c'
]

executable('unicode',
           sources,
           install : true,
           )
