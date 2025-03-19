include:
  - templates.salt-arc

{% from "variables.sls" import m,old_files with context %}
salt_master_packages:
  pkg.installed:
    - pkgs:
      #- xinetd
      - yandex-passport-vault-client
      - python-paramiko
      - python-requests

user-{{ m.robot }}:
  user.present:
    - name: {{ m.robot }}

/srv/salt/pillar_static_roots/stable/pillar:
  file.directory:
    - user: {{ m.robot }}
    - group: root
    - mode: 755
    - makedirs: True

/etc/salt/master:
  file.managed:
    - source: salt://master/master.conf
    - makedirs: True
    - user: {{ m.robot }}
    - template: jinja
    - context:
        user: {{ m.robot }}
        workers: {{ (grains['mem_total'] / 1000) | int }}

/etc/salt/pki/master/master.pem:
  file.managed:
    - user: {{ m.robot }}
    - contents_pillar: 'master:private_key'
    - mode: 400

salt_master_key_update:
  cmd.run:
    - name: openssl rsa -in /etc/salt/pki/master/master.pem -pubout -out /etc/salt/pki/master/master.pub && systemctl restart salt-master.service
    - onchanges:
      - file: /etc/salt/pki/master/master.pem

/etc/yandex/yav-deploy/pkg/salt-master/defaults.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://master/yav-deploy/pkg/salt-master/defaults.conf
    - template: jinja
    - context:
        salt_user: {{m.robot}}

/etc/yandex/yav-deploy/pkg/salt-master/templates/yav.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://master/yav-deploy/pkg/salt-master/templates/yav.conf

# generates /etc/salt/master.d/yav.conf file
# will fail if you logged in as root instead of sudo
yav-deploy /etc/salt/master.d/yav.conf:
  cmd.run:
    - name: /usr/bin/yav-deploy --debug

/usr/bin/salt-master-monrun-check.sh:
  file.managed:
    - mode: 755
    - source: salt://master/monrun.sh
  monrun.present:
    - name: salt-master
    - command: /usr/bin/salt-master-monrun-check.sh

# /etc/cron.d/salt-cron-jobs:
#   file.managed:
#     - source: salt://master/cron.jobs

/var/cache/salt/master:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - hidden_opts: size={{ m.tmpfs }}
    - opts: size={{ m.tmpfs }}
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True


/lib/systemd/system/salt-master.service:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        [Unit]
        Description=The Salt Master Server
        Documentation=man:salt-master(1) file:///usr/share/doc/salt/html/contents.html https://docs.saltstack.com/en/latest/contents.html
        After=network.target

        [Service]
        Type=simple
        LimitNOFILE=100000
        NotifyAccess=all
        ExecStart=/usr/bin/salt-master

        [Install]
        WantedBy=multi-user.target

chown -R {{m.robot}} /etc/salt /var/{cache,log,run}/salt/master:
  cmd.wait:
    - watch:
      - service: salt-master

chmod 0755 /var/log/salt:
  cmd.wait:
    - watch:
      - service: salt-master

# last state
salt-master:
  service.running:
    - enable: True
    - require:
      - pkg: salt_master_packages
      - mount: /var/cache/salt/master
    - watch:
      - file: /etc/salt/master
      - mount: /var/cache/salt/master

### cleanup
{% for of in old_files %}
{{of}}:
  file.absent: []
{% endfor %}
