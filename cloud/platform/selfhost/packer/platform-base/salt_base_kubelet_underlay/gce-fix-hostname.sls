Disable GCE hostname trimming:
  file.replace:
  - name: /usr/lib/python3/dist-packages/cloudinit/sources/DataSourceGCE.py
  # This is a horrible hack: we rename a method so that it no longer overrides a method in parent class.
  - pattern: 'def get_hostname\('
  - repl: 'def get_hostname_unused('
