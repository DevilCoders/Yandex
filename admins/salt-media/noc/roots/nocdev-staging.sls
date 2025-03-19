include:
  - templates.docker-ce
  - units.robot-nocdev-mysql
  - units.mysql.mysync-config
  - units.juggler-checks.common
  - units.juggler-checks.mysql-staging
  - units.chrony

/etc/default/config-caching-dns:
  file.managed:
    - makedirs: True
    - contents: NAT64="yes"
  cmd.run:
    - name: /usr/bin/config-caching-dns-update-configs
    - onchanges:
      - file: /etc/default/config-caching-dns

# iptables -t nat -A PREROUTING -d 172.18.0.1/32 -p tcp -m tcp --dport 18080 -j DNAT --to-destination 127.0.0.1:18080
rttestctl_nat:
  iptables.append:
    - table: nat
    - family: ipv4
    - chain: PREROUTING
    - destination: 172.18.0.1/32
    - proto: tcp
    - dport: 18080
    - jump: DNAT
    - to-destination: 127.0.0.1:18080
    - save: True
# ip6tables -t nat -A PREROUTING -d fd00:ffaa:a5ea::1/128 -p tcp -m tcp --dport 18080 -j DNAT --to-destination [::1]:18080
rttestctl_nat_v6:
  iptables.append:
    - use:
      - iptables: rttestctl_nat
    - family: ipv6
    - destination: fd00:ffaa:a5ea::1/128
    - jump: DNAT
    - to-destination: '[::1]:18080'

docker systemd options:
  file.managed:
    - name: /etc/systemd/system/docker.service.d/options.conf
    - makedirs: True
    - contents: |
        [Service]
        ExecStart=
        ExecStart=/usr/bin/dockerd -H unix://
  service.running:
    - name: docker
    - restart: True
    - require:
      - file: /etc/systemd/system/docker.service.d/options.conf
    - watch:
      - file: /etc/systemd/system/docker.service.d/options.conf

noc-gitlab rt-docker images:
  file.serialize:
    - name: /root/.docker/config.json
    - makedirs: True
    - dirmode: 700
    - mode: 400
    - serializer: json
    - serializer_opts:
      - indent: 2
    - dataset:
        auths:
          noc-gitlab.yandex-team.ru:5000:
            {% set user = salt["pillar.get"]("docker-auth:username") %}
            {% set passwd = salt["pillar.get"]("docker-auth:password") %}
            auth: {{ salt.hashutil.base64_encodestring(user + ":" + passwd) }}
  docker_image.present:
    - names:
      - consul:latest
      - robbertkl/ipv6nat:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-mysql:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-alexandria:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-cvs:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-fpm:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-nginx:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-test-tox:latest
      - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-dns:latest
    - require:
      - service: docker

racktables:
  user.present:
    - shell: /bin/bash
    - home: /home/racktables
    - usergroup: True
    - groups:
      - docker
      - www-data
  file.managed:
    - name: /etc/sudoers.d/rt-staging-zfs
    - mode: 440
    - user: root
    - group: root
    - contents: |
        racktables    ALL = NOPASSWD: /sbin/zfs *
        racktables    ALL = NOPASSWD: /bin/rm /rt-zpool/rt/mysql-*/auto.cnf

zfs rt states:
  zpool.present:
    - name: rt-zpool
    - config:
        import: false
        force: true
    - layout:
      {% if pillar['storage']['zfs_device'] is defined %}
        - {{ pillar['storage']['zfs_device'] }}
      {% else %}
        - mirror:
          - /dev/sda3
          - /dev/sdb3
        {% if salt.file.is_blkdev("/dev/sdc3") and salt.file.is_blkdev("/dev/sdd3") %}
        - mirror:
          - /dev/sdc3
          - /dev/sdd3
        {% endif %}
      {% endif %}
    - require_in:
      - zfs: zfs rt states
  cmd.run:
    - name: zfs allow -d racktables create,clone,snapshot,destroy,mount,userprop rt-zpool
    - onchanges:
      - user: racktables
      - zpool: rt-zpool
    - require:
      - user: racktables
      - zpool: rt-zpool
    - require_in:
      - zfs: zfs rt states

  {% set mysql_names = [ "mysql", "run-mysql" ] %}
  {% set app_names = [ "consul", "test-result", "test-report", "secrets", "cvs" ] %}
  zfs.filesystem_present:
    - create_parent: False
    - properties:
        setuid: off
    - names:
      - rt-zpool/rt
      {% for name in mysql_names + app_names %}
      - rt-zpool/rt/{{name}}
      {% endfor %}

  file.directory:
    - user: racktables
    - group: racktables
    - names:
      - /rt-zpool/rt:
        - require:
          - zfs: rt-zpool/rt
      {% for name in app_names %}
      - /rt-zpool/rt/{{name}}:
        - require:
          - zfs: rt-zpool/rt/{{name}}
      {% endfor %}
      {% for name in mysql_names %}
      - /rt-zpool/rt/{{name}}:
        - user: 999
        - group: 999
        - require:
          - zfs: rt-zpool/rt/{{name}}
      {% endfor %}

rttestctl:
  pkg.installed:
    - pkgs:
      - rttestctl
      - yandex-passport-tvmtool
    - require:
      - zpool: rt-zpool
  file.managed:
    - makedirs: True
    - dirmode: 755
    - user: racktables
    - group: racktables
    - require:
      - user: racktables
      - pkg: rttestctl
    - names:
      - /etc/rttestctl/config.secret.yml:
        - mode: 400
        {% set syml = pillar["rttestctl"]["secrets"] %}
        - contents: |
            gitlab_token: "{{syml["GITLAB_TOKEN"]}}"
            consul:
              encrypt: "{{syml["CONSUL_ENCRYPT"]}}"
            db:
              db_dsn: "root:{{syml["mysql_root"]}}@unix(./run-mysql/mysqld/mysqld.sock)/racktables"
              racktables_pwd: "{{syml["RT_PWD"]}}"
              racktables_ro_pwd: "{{syml["RT_RO_PWD"]}}"
      - /etc/rttestctl/config.yml:
        # не берем тестовые ноды стейджинга
        {% set nodes = salt.conductor.groups2hosts("nocdev-staging") %}
        - contents: |
            base_url: "https://{{salt["pillar.get"]("rttestctl:cluster:"+grains["host"]+":cname")}}"
            dc: "{{grains['conductor']['root_datacenter']}}"
            log_level: "debug"
            cvs:
              enable: true
            consul:
              service_name: "{{pillar["rttestctl"]["consul"]["service_name"]}}"
              weight: {{salt["pillar.get"]("rttestctl:cluster:"+grains["host"]+":weight")}}
              retry_join:
                {%- for node in nodes %}
                - {{ node }}
                {%- endfor %}
            images:
              mysql:
                - noc-gitlab.yandex-team.ru:5000/nocdev/rt-docker/rt-mysql:latest
            projects:
              rt:
                name: "rt"
                default_project: "nocdev/racktables"
                env_prefix: "RT_YANDEX"
              invapi:
                name: "invapi"
                default_project: "nocdev/invapi"
                env_prefix: "RT_INVAPI"
              rtapi:
                name: "rtapi"
                default_project: "nocdev/rtapi2rr"
                env_prefix: "RT_RTAPI"
            run_cmd:
              rtapi: auto
              fpm: auto
              test: auto
      - /usr/share/rttestctl/Corefile:
        - contents: |
            .:53 {
              docker unix:///var/run/docker.sock {
                domain d.loc
                hostname_domain d-host.loc
              }
              forward . [2a02:6b8:0:3400::5005]:53
              cache 300
              errors
              log
            }
  service.running:
    - enable: True
    - names:
      - yandex-passport-tvmtool:
        - require:
          - pkg: rttestctl
          - file: tvm-config
      - rttestctl:
        - require:
          - file: rttestctl
          - pkg: rttestctl
          - file: cvs-secrets
          - file: rt-secrets

tvm-config:
  file.serialize:
    - name: /etc/tvmtool/tvmtool.conf
    - makedirs: True
    - dirmode: 700
    - mode: 440
    - group: www-data
    - serializer: json
    - serializer_opts:
      - indent: 2
    - require:
      - pkg: rttestctl
    {% set stvm = pillar["rttestctl"]["secrets"] %}
    - dataset:
        BbEnvType: 2
        port: 18080
        clients:
          alexandria:
            secret: "{{ stvm["TVM_ALEXANDRIA_DEV"] }}"
            self_tvm_id: 2025424
            dsts: { blackbox: { dst_id: 223 }, racktables: { dst_id: 2020198 }}
          racktables:
            secret: "{{ stvm["TVM_RT"] }}"
            self_tvm_id: 2020198
            dsts: { blackbox: { dst_id: 223 }}
          rtctl:
            secret: "{{ stvm["TVM_RTCTL"] }}"
            self_tvm_id: 2020170
            dsts: { blackbox: { dst_id: 223 }}

cvs-secrets:
  file.managed:
    # - mode: 400
    - mode: 444 # Почему так выставлено в конфиге yav-deploy???
    - user: racktables
    - group: racktables
    - makedirs: True
    - require:
      - zfs: zfs rt states
      - user: racktables
    - names:
      {% for name, value in pillar["rttestctl"]["cvs-secrets"].items() %}
      - /rt-zpool/rt/secrets/cvs/{{name}}:
        - contents: |
            {{value.replace("\\n", "\n")|indent(12)}}
      {% endfor %}

rt-secrets:
  file.managed:
    - mode: 400
    - user: racktables
    - group: racktables
    - makedirs: True
    - require:
      - zfs: zfs rt states
      - user: racktables
    - names:
      - /rt-zpool/rt/secrets/id_rsa_racktables.pub:
        - contents_pillar: "rttestctl:secrets:id_rsa_racktables.pub"
        - mode: 444
      - /rt-zpool/rt/secrets/id_rsa_racktables:
        - contents_pillar: "rttestctl:secrets:id_rsa_racktables"
      - /rt-zpool/rt/secrets/ssh_id_rsa:
        - contents_pillar: "rttestctl:secrets:ssh_id_rsa"
      - /rt-zpool/rt/secrets/cert.pem:
        - contents_pillar: "rttestctl:rt-cert"
      - /rt-zpool/rt/secrets/arc-nocdev-token:
        - contents_pillar: "rttestctl:arc-secret"

add links to mysync config and secrets:
  file.symlink:
    - makedirs: True
    - names:
      - /rt-zpool/rt/secrets/mysync.yaml:
        - target: /etc/mysync.yaml
      - /rt-zpool/rt/secrets/id_rsa_robot-nocdev-mysql:
        - target: /home/robot-nocdev-mysql/.ssh/id_rsa
    - require:
      - zfs: zfs rt states
      - pkg: rttestctl
    - require_in:
      - service: rttestctl

/rt-zpool/rt/secrets/root-my.cnf:
  file.managed:
    - mode: 400
    - user: racktables
    - makedirs: True
    - contents: |
        [client]
        user=admin
        host=localhost
        port=3306
        password={{ pillar['mysql_secrets']['mysync_admin_pas'] }}

        [mysql]
        default-character-set=utf8mb4
    - require:
      - zfs: zfs rt states
    - require_in:
      - service: rttestctl

rt-mysql-main-config:
  cmd.run:
    {% set server_id = pillar['mysql']['server-ids'][grains['host']] %}
    - name: sed 's/@@SERVER_ID@@/{{server_id}}/' /usr/share/rttestctl/custom.cnf.example > /usr/share/rttestctl/custom.cnf
    - unless: diff -U0 <(sed 's/@@SERVER_ID@@/{{server_id}}/' /usr/share/rttestctl/custom.cnf.example) /usr/share/rttestctl/custom.cnf
    - require:
      - pkg: rttestctl
    - require_in:
      - service: rttestctl

/etc/telegraf/telegraf.d/mysql.conf:
  file.managed:
    - makedirs: True
    - source: salt://units/mysql/files/etc/telegraf/telegraf.d/mysql.conf
    - template: jinja
    - context:
        mysocket: "/rt-zpool/rt/run-mysql/mysqld/mysqld.sock"
    - mode: 640
    - group: telegraf
    - require:
      - zfs: zfs rt states
  pkg.installed:
    - name: telegraf
    - require:
      - file: /etc/telegraf/telegraf.d/mysql.conf

{% if pillar['recreate_env'] is defined and pillar['recreate_env'] %}
/usr/local/sbin/rttctl:
  file.managed:
    - source: https://test.racktables.yandex-team.ru/v1/client/linux
    - skip_verify: True
    - mode: 755

/etc/cron.d/recreate_rt_env:
  file.managed:
    - contents: |
        0 7 * * * root env RTTCTL_HOST={{salt["pillar.get"]("rttestctl:cluster:"+grains["host"]+":cname")}} /usr/local/sbin/rttctl spawn &>/var/log/cron.recreate_rt_env.last

/root/.config/rttctl/token:
  file.managed:
    - makedirs: True
    - contents_pillar: "rttestctl:secrets:rttctl_token"

/etc/monrun/salt_actual_env/actual_env.sh:
  file.managed:
    - makedirs: True
    - mode: 755
    - contents: |
        #!/bin/bash

        export HOME=/root RTTCTL_HOST={{salt["pillar.get"]("rttestctl:cluster:"+grains["host"]+":cname")}}
        max_ts=$( /usr/local/sbin/rttctl ps --json | jq --arg host $RTTCTL_HOST '.[] | select(.Node.host==$host) | .Envs[].endpoints.deleteAt' | xargs -n1  date +%s --date | sort -nr | head -n1 )
        if [ -z "$max_ts" ]; then
          echo "PASSIVE-CHECK:actual_env;2;No any staging environment" | tee /tmp/actual_env.last
          exit 0
        fi

        now_ts=$(date +%s)

        if [  `expr $max_ts - $now_ts` -lt 86400 ]; then
          echo "PASSIVE-CHECK:actual_env;1;No staging environment for tomorrow"
          exit 0
        fi

        echo "PASSIVE-CHECK:actual_env;0;OK"
  pkg.installed:
    - name: jq

/etc/monrun/salt_actual_env/MANIFEST.json:
  file.managed:
    - makedirs: True
    - contents: |
        {
            "version": 1,
            "checks": [{
                "check_script": "/etc/monrun/salt_actual_env/actual_env.sh",
                "interval": 600,
                "run_always": true,
                "services": [
                    "actual_env"
                ],
                "timeout": 30
            }]
        }
{% endif %}

