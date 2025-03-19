l2_sudoers:
  file.accumulated:
    - filename: /etc/sudoers.d/l2_support
    - text:
        - 'ALL = NOPASSWD: ALL'
    - require_in:
      - file: /etc/sudoers.d/l2_support
