tigervncserver:
  pkg.installed:
    - sources:
      - tigervncserver: https://bintray.com/artifact/download/tigervnc/stable/ubuntu-14.04LTS/amd64/tigervncserver_1.6.0-3ubuntu1_amd64.deb

vncserver:
  service.running:
    - enable: True
    - require:
      - file: /home/cores/.vnc/xstartup

/etc/default/vncservers:
  file.managed:
    - source: salt://{{ slspath }}/vncservers
    - require:
      - pkg: tigervncserver
      - user: cores

/home/cores/.vnc/config:
  file.managed:
    - user: cores
    - group: cores
    - mode: 644
    - makedirs: True
    - dir_mode: 755
    - contents: |
        geometry=1920x1080
    - require:
      - pkg: tigervncserver
      - user: cores

/home/cores/.vnc/xstartup:
  file.managed:
    - source: salt://{{ slspath }}/xstartup
    - user: cores
    - group: cores
    - mode: 755
    - makedirs: True
    - dir_mode: 755
    - require:
      - pkg: tigervncserver
      - user: cores

/home/cores/.vnc/passwd:
  file.managed:
    - source: salt://{{ slspath }}/passwd  # fVNJjL1E <- this is the password
    - user: cores
    - group: cores
    - mode: 600
    - makedirs: True
    - dir_mode: 755
    - require:
      - pkg: tigervncserver
      - user: cores
