gfglister:
  user.present:
    - name: gfglister
    - system: True
  pkg.latest:
    - refresh: true
  service.running:
    - enable: True

/etc/gfglister/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/gfglister/config.yaml
    - template: jinja
    - user: gfglister
    - group: gfglister
    - mode: 400
    - dir_mode: 750
    - makedirs: True

/etc/gfglister/ssh_identity:
  file.managed:
    - user: gfglister
    - group: gfglister
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'comocutor:identity'

/etc/gfglister/racktables_id_dsa:
  file.managed:
    - user: gfglister
    - group: gfglister
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'comocutor:identity_racktables'

/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'unified_agent:tvm-client-secret'

/etc/yandex/unified_agent/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/conf.d
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - file_mode: 644
    - dir_mode: 755
    - makedirs: True
    - clean: True

/etc/telegraf/telegraf.d/gfglister.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/telegraf.d/gfglister.conf
    - user: telegraf
    - group: telegraf
    - mode: 600
    - makedirs: True

annushkarepo:
  git.latest:
    - name: https://noc-gitlab.yandex-team.ru/nocdev/annushka.git
    - target: /home/gfglister/annushka
    - user: gfglister

annushkavenv:
  pkg.installed:
    - pkgs:
      - python3-venv
      - python3-setuptools
  cmd.run:
    - name: python3 -m venv /home/gfglister/venv
    - runas: gfglister

annushka-prereq:
  cmd.run:
    - name: /home/gfglister/venv/bin/python3 -m pip install -i https://pypi.yandex-team.ru/simple/ -U dataclasses setuptools-rust pip
    - runas: gfglister

annushka-req:
  cmd.run:
    - name: /home/gfglister/venv/bin/python3 -m pip install -i https://pypi.yandex-team.ru/simple/ -r /home/gfglister/annushka/requirements.txt
    - runas: gfglister

packages:
  pkg.installed:
    - pkgs:
        - subversion
