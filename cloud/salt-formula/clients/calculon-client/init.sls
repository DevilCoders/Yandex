{{ sls }}-pkgs:
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

{{ sls }}:
  file.recurse:
    - name: /usr/share/calculon-client/
    - source: salt://{{ slspath }}/code/
    - file_mode: '0755'
    - user: root
    - group: root

{{ sls }}-cfg:
  file.managed:
    - name: /etc/calculon-client/calculon-client.yaml
    - source: salt://{{ slspath }}/etc/calculon-client.yaml
    - template: jinja
    - makedirs: True

{{ sls }}-tpl:
  file.managed:
    - name: /etc/calculon-client/io_tests.tpl
    - source: salt://{{ slspath }}/templates/io_tests.tpl
    - makedirs: True


{{ sls }}-logdir:
  file.directory:
    - name: /var/log/calculon-client
    - makedirs: True

{{ sls }}-fio_configs:
  file.directory:
    - name: /opt/fio/jobs
    - makedirs: True

{{ sls }}-fio_dumps:
  file.directory:
    - name: /opt/fio/dumps
    - makedirs: True

