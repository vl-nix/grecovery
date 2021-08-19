project('libphotorec', 'c', version: '21.0')

c_args = ['-DHAVE_CONFIG_H']
incdir = include_directories('../../include')

cc = meson.get_compiler('c')

libz = cc.find_library('libz', required: true)

c = run_command('sh', '-c', 'for file in src/*.c; do echo $file; done')
lib_src = c.stdout().strip().split('\n')

lib_dep  = [dependency('ext2fs'), dependency('libntfs-3g'), dependency('libjpeg'), libz]

lib_both = both_libraries('photorec', sources: lib_src, include_directories: incdir, dependencies: lib_dep, c_args: c_args)

