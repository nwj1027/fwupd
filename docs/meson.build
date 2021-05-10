toml_conf = configuration_data()
toml_conf.set('version', meson.project_version())

fwupd_toml = configure_file(
  input: 'fwupd.toml.in',
  output: 'fwupd.toml',
  configuration: toml_conf
)

fwupdplugin_toml = configure_file(
  input: 'fwupdplugin.toml.in',
  output: 'fwupdplugin.toml',
  configuration: toml_conf
)

gidocgen = find_program('gi-docgen')

custom_target('doc-fwupd',
  input: [
    fwupd_toml,
    fwupd_gir[0],
  ],
  output: 'libfwupd',
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../libfwupd'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depends: [
    fwupd_gir[0],
  ],
  build_by_default: true,
  install: true,
  install_dir: join_paths(datadir, 'doc', 'fwupd'),
)

custom_target('doc-fwupdplugin',
  input: [
    fwupdplugin_toml,
    fwupdplugin_gir[0],
  ],
  output: 'libfwupdplugin',
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../libfwupdplugin'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depends: [
    fwupdplugin_gir[0],
  ],
  build_by_default: true,
  install: true,
  install_dir: join_paths(datadir, 'doc', 'fwupd'),
)
