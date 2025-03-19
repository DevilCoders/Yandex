{{ sls }}_pkgs:
  pkg.installed:
    - pkgs:
      - fio
      - python-jinja2
      - python-requests
      - zlib1g-dev
      - libbz2-dev
      - libguestfs-tools
      - qemu-kvm
      - git
      - ycu-calculator

{{ sls }}_code:
  file.recurse:
    - name: /usr/share/calculon-client/
    - source: salt://{{ slspath }}/code/
    - file_mode: '0755'
    - user: root
    - group: root

{{ sls }}_lib:
  file.recurse:
    - name: /usr/share/calculon-client/
    - source: salt://{{ slspath }}/lib

{{ sls }}_settings:
  file.recurse:
    - name: /usr/share/calculon-client/
    - source: salt://calculon/etc
    - template: jinja

{{ sls }}_tpl:
  file.synlink:
    - name: /etc/calculon-client/io_tests.tpl
    - target: /usr/share/calculon-client/tests.tpl
    - makedirs: True
    - require:
     - file: {{ sls }}_settings

{{ sls }}-log:
  file.touch:
    - name: /var/log/calculon/client.log
    - makedirs: True
