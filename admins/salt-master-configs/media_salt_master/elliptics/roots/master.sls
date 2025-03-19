{% set robot = salt['pillar.get']('master:user') %}

yav_pkgs:
  pkg.installed:
    - pkgs:
      - yandex-passport-vault-client
      - yandex-arc-launcher

/etc/salt/pki/master/master.pem:
  file.managed:
    - makedirs: True
    - user: {{ robot }}
    - contents: |-
        {{ salt['pillar.get']('master:private_key')|indent(8) }}
    - mode: 400

salt_master_key_update:
  cmd.run:
    - name: openssl rsa -in /etc/salt/pki/master/master.pem -pubout -out /etc/salt/pki/master/master.pub && systemctl restart salt-master.service
    - onchanges:
      - file: /etc/salt/pki/master/master.pem

salt_master:
  file.managed:
    - user: {{ robot }}
    - group: root
    - makedirs: True
    - names:
      - /etc/salt/master.d/master.conf:
        - contents: |
            auto_accept: true
            interface: '::'
            ipv6: true
            keep_jobs: 1
            log_level: INFO
            log_level_logfile: INFO
            open_mode: True
            renderer: jinja | yaml | gpg
            user: {{ robot }}
            worker_threads: {{ (grains['mem_total'] / 1000) | int }}
            # NOCDEV-6215 все extmods вкомпилены в бинарь салта, поэтому даже не пытаемся их тянуть с FS
            # https://docs.saltproject.io/en/latest/ref/configuration/master.html#extmod-whitelist-extmod-blacklist
            extmod_whitelist: {
              cache: [], clouds: [], engines: [], grains: [], modules: [],
              output: [], pillar: [], proxy: [], queues: [], renderers: [],
              returners: [], roster: [], runners: [], sdb: [], states: [],
              tokens: [], tops: [], utils: [], wheel: [],
            }

            reactor:
            - salt/minion/*/start:
              - salt://start.sls

            top_file_merging_strategy: same
            dynamic_roots_config: /srv/salt/configs/dynamic_roots_config.yaml
            fileserver_backend:
            - dynamic_roots

            pillar_source_merging_strategy: none
            dynamic_pillar_config: /srv/salt/configs/dynamic_pillar_config.yaml
            pillarenv_from_saltenv: true
            ext_pillar:
            - dynamic_pillar_roots: '~'
      - /lib/systemd/system/salt-master.service:
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

  pkg.installed:
    - name: yandex-salt-components
  service.running:
    - name: salt-master
    - enable: True

/etc/yandex/salt/arc2salt.yaml:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - source: salt://{{ slspath }}/files/arc2salt.yaml
    - template: jinja

/etc/cron.d/arc2salt:
  file.managed:
    - contents: |
       */2 * * * * {{ robot }}  sudo /usr/bin/salt-arc --debug --log-file /var/log/arc2salt.log
       0 0 * * 0 {{ robot }} sudo /usr/bin/salt-arc --arc-gc --cleanup --log-file /var/log/arc2salt.log
    - user: root
    - group: root
    - mode: 644

salt-master:
  monrun.present:
    - command: "/usr/bin/salt-arc --check --log-file /var/log/arc2salt.log"
    - execution_interval: 60
    - execution_timeout: 30

/home/{{ robot }}/.arc/token:
  file.managed:
    - mode: 0600
    - makedirs: True
    - user: {{ robot }}
    - group: dpt_virtual_robots
    - contents: {{ salt['pillar.get']('arc2salt:arc-token') | json }}

/var/log/salt:
  file.directory:
    - user: {{ robot }}
    - group: root
    - mode: 755
    - makedirs: True
