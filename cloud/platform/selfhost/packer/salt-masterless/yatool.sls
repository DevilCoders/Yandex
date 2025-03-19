Install yatool into system dir:
  file.managed:
    - name: /usr/local/bin/ya
    - source: salt://ya
    - mode: 755

Download yc binary:
  cmd.run:
    - name: /usr/local/bin/ya
