scripts:
  file.recurse:
    - name: /usr/local/bin
    - source: salt://{{ slspath }}/files/usr/local/bin
    - dir_mode: 755
    - file_mode: 755
