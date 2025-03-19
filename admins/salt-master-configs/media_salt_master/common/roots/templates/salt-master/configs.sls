{% from slspath + "/map.jinja" import salt_master with context %}
include:
  - .services

/etc/salt:
  file.directory:
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - dir_mode: 755
    - recurse:
      - user

/srv/salt:
  file.directory:
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - dir_mode: 750
    - recurse:
      - user

/etc/salt/master:
  file.managed:
    - user: {{ salt_master.params.user }}

{% if salt_master.ssh %}
/home/{{ salt_master.params.user }}/.ssh:
  file.directory:
    - user: {{ salt_master.params.user }}
    - group: dpt_virtual_robots
    - dir_mode: 0700
    - makedirs: True

{% if salt_master.ssh.key is defined %}
/home/{{ salt_master.params.user }}/.ssh/id_rsa:
  file.managed:
    - mode: 0600
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - group: dpt_virtual_robots
    - contents: {{ salt_master.ssh.key | json }}
{% else %}
{% if salt_master.ssh.key_repo is defined %}
/usr/local/bin/salt-ssh-key-get.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/get-ssh-keys/get.sh
    - template: jinja
    - mode: 755
    - makedirs: True
    - context:
      ssh: {{ salt_master.ssh }}
      user: {{ salt_master.params.user }}

get-robot-ssh-keys:
  cmd.run:
    - name: /usr/local/bin/salt-ssh-key-get.sh
    - cwd: /home/{{ salt_master.params.user }}
{% else %}
get_robot_ssh_keys:
  cmd.run:
    - name: scp $LC_USER@secdist.yandex.net:{{ salt_master.ssh.key_path }}/{{ salt_master.ssh.key_name }} /home/{{ salt_master.params.user }}/.ssh/id_rsa
    - creates: /home/{{ salt_master.params.user }}/.ssh/id_rsa
{% endif %}

fix_robot_ssh_keys_permissions:
  file.managed:
    - name: /home/{{ salt_master.params.user }}/.ssh/id_rsa
    - mode: 0600
    - user: {{ salt_master.params.user }}
    - group: dpt_virtual_robots
{% endif %}
{% endif %}

/etc/salt/master.d/master.conf:
  file.managed:
    - source: {{ salt_master.config }}
    - template: jinja
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - context:
      params: {{ salt_master.params }}
      senv: {{ salt_master.senv }}
      project: {{ salt_master.project }}
      default: {{ salt_master.default }}
{% if salt_master.file_roots is defined %}
      file_roots: {{ salt_master.file_roots }}
{% endif %}
{% if salt_master.gitfs_remotes is defined %}
      gitfs_remotes: {{ salt_master.gitfs_remotes }}
{% endif %}
{% if salt_master.ext_pillar is defined %}
      ext_pillar: {{ salt_master.ext_pillar }}
{% endif %}
      ssh: {{ salt_master.ssh }}
    - watch_in:
      - service: {{ salt_master.service }}

{% if salt_master.masterd %}
/etc/salt/master.d:
  file.recurse:
    - source: {{ salt_master.masterd }}
    - user: {{ salt_master.params.user }}
    - dir_mode: 755
    - file_mode: 644
{% endif %}

/etc/salt/gpgkeys:
  file.directory:
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - file_mode: 600
    - dir_mode: 700
    - recurse:
      - user
      - mode

{% if salt_master.git_local %}
/usr/local/bin/salt-git-update.sh:
  file.managed:
    - mode: 755
    - source: salt://{{ slspath }}/files/git-local/salt-git-update.sh
    - template: jinja
    - context:
      git_local: {{ salt_master.git_local | yaml }}
      user: {{ salt_master.params.user }}
  cmd.wait:
    - require:
      - file: /usr/local/bin/salt-git-update.sh

/usr/local/bin/salt-master-monrun-check.sh:
  file.managed:
    - template: jinja
    - mode: 755
    - source: salt://{{ slspath }}/files/git-local/salt-master-monrun-check.sh
    - context:
      salt_master: {{ salt_master | yaml }}
  monrun.present:
    - name: salt-master
    - command: /usr/local/bin/salt-master-monrun-check.sh

/etc/cron.d/salt-git-update:
  file.managed:
    - source: salt://{{ slspath }}/files/git-local/salt-git-update.cron

{% endif %}

{% if salt_master.csync2 %}
salt_master_packages:
  pkg.latest:
    - pkgs:
      - xinetd
      - csync2
{% if salt_master.csync2.from_pillar is defined %}
{% for key, value in salt_master.csync2.from_pillar.items() %}
/etc/{{ key }}:
  file.managed:
    - mode: 0644
    - makedirs: True
    - user: root
    - group: root
    - contents: {{ value | json }}
{% endfor %}
{% elif salt_master.csync2.secdist_path is defined %}
get-csync2-secrets:
  cmd.run:
    - name: scp $LC_USER@secdist.yandex.net:{{ salt_master.csync2.secdist_path }}/csync2{.key,_ssl_{key,cert}.pem} /etc/
{% else %}
/usr/local/bin/salt-csync2-secrets-get.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/csync2/get-secrets.sh
    - template: jinja
    - mode: 755
    - makedirs: True
    - context:
      csync2: {{ salt_master.csync2 }}
      user: {{ salt_master.params.user }}

get-csync2-secrets:
  cmd.run:
    - name: /usr/local/bin/salt-csync2-secrets-get.sh
    - cwd: /home/{{ salt_master.params.user }}
{% endif %}

/etc/csync2.cfg:
  file.managed:
    - makedirs: True
    - mode: 644
    - template: jinja
    - source: salt://{{ slspath }}/files/csync2/csync2.cfg
    - context:
      masters: {{ salt.conductor.groups2hosts(salt['grains.get']('conductor:group')) }}
/etc/logrotate.d/salt-other:
  file.managed:
    - source: salt://{{ slspath }}/files/csync2/logrotate.conf

/etc/xinetd.d/zope_csync2:
  file.managed:
    - source: salt://{{ slspath }}/files/csync2/inetd/salt-xinetd.conf
/usr/local/bin/salt-inetd-handler.sh:
  file.managed:
    - mode: 755
    - source: salt://{{ slspath }}/files/csync2/inetd/handler.sh
service --status-all|&awk '/inetd$/{print $NF}'|xargs --no-run-if-empty -I_ service _ restart:
  cmd.wait:
    - watch:
      - file: /etc/xinetd.d/zope_csync2

/etc/cron.d/salt-csync2:
  file.managed:
    - source: salt://{{ slspath }}/files/csync2/salt-csync2.cron
{% endif %}

{% if salt_master.arcadia is defined %}
/home/{{ salt_master.params.user }}/.arc/token:
  file.managed:
    - mode: 0600
    - makedirs: True
    - user: {{ salt_master.params.user }}
    - group: dpt_virtual_robots
    - contents: {{ salt_master.arcadia.arc_token | json }}

/usr/local/bin/deploy-configs.py:
  file.managed:
    - mode: 755
    - source: salt://{{slspath }}/files/arcadia/deploy-configs.py

/etc/cron.d/salt-arc-update:
  file.managed:
    - source: salt://{{ slspath }}/files/arcadia/salt-arc-update.cron
    - template: jinja
    - context:
      remote_path: {{ salt_master.arcadia.remote_path }}
      local_target: {{ salt_master.arcadia.local_target }}
      salt_user: {{ salt_master.params.user }}
{% endif %}

