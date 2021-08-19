project('grecovery', 'c', version: '21.8')

desktop = meson.project_name() + '.desktop'
bindir = join_paths(get_option('prefix'), get_option('bindir'))

lib_proj = subproject('lib')
lib_both = lib_proj.get_variable('lib_both')

cc = meson.get_compiler('c')

c_args = ['-DHAVE_CONFIG_H']
incdir = include_directories('include')

gnome = import('gnome')
res = gnome.compile_resources('res', join_paths('data', 'icons', 'gresource.xml'), source_dir: join_paths('data', 'icons'))

dep = [dependency('gtk+-3.0', version: '>= 3.22')]

c = run_command('sh', '-c', 'for file in gtk/src/*.c; do echo $file; done')
src = c.stdout().strip().split('\n')
src += res

executable(meson.project_name(), src, include_directories: incdir, dependencies: dep, link_with: lib_both.get_static_lib(), c_args: c_args, install: true)

configure_file(input: join_paths('data', 'desktop'), output: desktop, copy: true, install: true, install_dir: join_paths('share', 'applications'))
configure_file(input: join_paths('data', 'grecovery-pkexec'), output: 'grecovery-pkexec', copy: true, install: true, install_dir: bindir)

