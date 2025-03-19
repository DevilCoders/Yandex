remove ipv4 only:
  file.absent:
    - name: /etc/apt/apt.conf.d/99ipv4-only

