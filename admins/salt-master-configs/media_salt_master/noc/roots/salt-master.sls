{% set robot = salt['pillar.get']('master:user') %}

/usr/lib/monrun/checks/salt-master.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - source: salt://files/salt-master/salt-master.sh

/etc/monrun/conf.d/salt-master.conf:
  file.managed:
    - user: root
    - group: root
    - source: salt://files/salt-master/salt-master.conf

/etc/salt/pki/master/master.pem:
  file.managed:
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
            transport: tcp        # NOCDEV-7503
            keep_jobs: 1
            log_level: INFO
            log_level_logfile: INFO
            open_mode: True
            renderer: jinja | yaml
            user: {{ robot }}
            worker_threads: 24
            # NOCDEV-6215 все extmods вкомпилены в бинарь салта, поэтому даже не пытаемся их тянуть с FS
            autoload_dynamic_modules: false 
            #reactor:  # реактор мы не используем, может быть когда-нибудь потом может быть всё же...
            #- salt/minion/*/start:
            #  - salt://start.sls

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
