{{ sls }}_pkgs:
  pkg.installed:
    - pkgs:
      - build-essential
      - python-dev
      - python-pip

{{ sls }}_pips:
  pip.installed:
    - name: virtualenv

{{ sls }}_work_dir:
  virtualenv.managed:
    - name: /usr/share/calculon/
    - pip_exists_action: s
    - pip_pkgs:
      - flask
      - requests

{{ sls }}_socket_dir:
  file.directory:
    - name: /var/run/{{ sls }}/
    - user: www-data
    - group: www-data

{{ sls }}_log_dir:
  file.directory:
    - name: /var/log/{{ sls }}/
    - user: www-data
    - group: www-data
    - makedirs: True
