cores:
  user.present:
    - shell: /bin/bash
    - home: /home/cores
    - password: $1$slkfdslk$qmgekFnecdJ1HeZ5qjRm5/  # ghwGFc9O <- this is the password

show-desktop-icons.desktop:
  file.managed:
    - name: /home/cores/.config/autostart/show-desktop-icons.desktop
    - source: salt://{{ slspath }}/show-desktop-icons.desktop
    - makedirs: True
    - user: cores
    - group: cores
    - mode: 700
    - require:
      - user: cores

show-desktop-icons.sh:
  file.managed:
    - name: /home/cores/bin/show-desktop-icons.sh
    - source: salt://{{ slspath }}/show-desktop-icons.sh
    - makedirs: True
    - user: cores
    - group: cores
    - mode: 700
    - require:
      - user: cores
