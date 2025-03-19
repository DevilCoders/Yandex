monrun-regenerate:
  cmd.wait:
    - name: 'monrun --gen-jobs && /etc/init.d/snaked reconfigure'
    - cwd: /
