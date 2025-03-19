breakglass:
  user.present:
    - shell: /bin/bash
    - home: /home/breakglass

/etc/sudoers.d/breakglass:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - contents: 'breakglass ALL=(ALL) NOPASSWD:ALL'
    - require:
      - user: breakglass

