{{ sls }}:
  user.present:
    - shell: /bin/bash
    - home: /home/{{ sls }}

/etc/sudoers.d/{{ sls }}:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - contents: '{{ sls }} ALL=(ALL) NOPASSWD:ALL'
    - require:
      - user: {{ sls }}
