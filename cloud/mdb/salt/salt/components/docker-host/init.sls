include:
    - components.repositories.apt.docker

go-packages:
    pkg.installed:
        - pkgs:
            - golang-1.10

go-symlink:
    file.symlink:
        - name: /usr/local/bin/go
        - target: /usr/lib/go-1.10/bin/go
        - require:
            - pkg: go-packages

docker-pkgs:
    pkg.installed:
        - pkgs:
            - docker-ce: '5:19.03.5~3-0~ubuntu-bionic'
            - libpython3.6: 'latest'
            - libpython3.6-minimal: 'latest'
            - libpython3.6-stdlib: 'latest'
            - python3.6: 'latest'
            - python3.6-minimal: 'latest'
            - python3.6-venv: 'latest'
            - libmpdec2: 'latest'
            - python-requests: 'latest'
            - openjdk-17-jre-headless: 'latest'
            - build-essential
            - virtualenv
            - python3-dateutil
            - python2.7-dev
            - python-virtualenv
            - python3-requests
            - libffi-dev
            - haveged
            - devscripts
            - fakeroot
            - yandex-internal-root-ca
            - debootstrap
            - yum
            - s3cmd
            - unzip
        - require:
            - pkgrepo: docker-repo
            - file: /etc/docker/daemon.json

unused-docker-pkgs:
    pkg.purged:
        - pkgs:
            - golang-docker-credential-helpers
            - pass
            - openjdk-8-jre-headless
            - yandex-jdk8-unlimited-jce
            - openjdk-11-jre-headless
        - require:
            - pkg: docker-pkgs

libmysqlclient-dev-pkgs:
    pkg.installed:
        - pkgs:
            - libmysqlclient-dev

odbc-packages:
    pkg.installed:
        - pkgs:
            - unixodbc: 2.3.7+yandex0
            - unixodbc-dev: 2.3.7+yandex0
            - odbcinst1debian2: 2.3.7+yandex0
            - libodbc1: 2.3.7+yandex0
            - odbcinst: 2.3.7+yandex0
            - msodbcsql17: 17.4.2.1-1+yandex0
            - python-pyodbc

create-tox-venv:
    cmd.run:
        - name: pyvenv-3.6 /opt/tox
        - require:
            - pkg: docker-pkgs
        - unless:
            - /opt/tox/bin/pip freeze

install-tox:
    cmd.run:
        - name: /opt/tox/bin/pip install tox
        - require:
            - cmd: create-tox-venv
        - unless:
            - /opt/tox/bin/pip freeze | grep tox

install-retrying:
    cmd.run:
        - name: /opt/tox/bin/pip install retrying
        - require:
            - cmd: create-tox-venv
        - unless:
            - /opt/tox/bin/pip freeze | grep retrying

install-jinja2:
    cmd.run:
        - name: /opt/tox/bin/pip install jinja2
        - require:
            - cmd: create-tox-venv
        - unless:
            - /opt/tox/bin/pip freeze | grep -i jinja2

tox-symlink:
    file.symlink:
        - name: /usr/local/bin/tox
        - target: /opt/tox/bin/tox
        - require:
            - cmd: install-tox

create-ansible-juggler-venv:
    cmd.run:
        - name: /usr/bin/virtualenv /opt/ansible-juggler
        - require:
            - pkg: docker-pkgs
        - unless:
            - /opt/ansible-juggler/bin/pip freeze

install-ansible-juggler:
    cmd.run:
        - name: |
            /opt/ansible-juggler/bin/pip install -U six
            /opt/ansible-juggler/bin/pip install -U setuptools
            /opt/ansible-juggler/bin/pip install ansible-juggler -i https://pypi.yandex-team.ru/simple
        - require:
            - cmd: create-ansible-juggler-venv
        - unless:
            - /opt/ansible-juggler/bin/pip freeze | grep ansible-juggler

suitable-pip:
    cmd.run:
        - name: /opt/tox/bin/pip install --upgrade pip==21.0.1
        - require:
              - cmd: create-tox-venv
        - unless:
            - /opt/tox/bin/pip -V | grep -q ' 21.0.1 '

install-docker-compose:
    cmd.run:
        - name: /opt/tox/bin/pip install -U docker-compose==1.29.2
        - require:
            - cmd: suitable-pip
        - unless:
            - /opt/tox/bin/pip freeze | grep -q docker-compose==1.29.2

install-dateutil:
    cmd.run:
        - name: /opt/tox/bin/pip install -U python-dateutil==2.8.2
        - require:
            - cmd: suitable-pip
        - unless:
            - /opt/tox/bin/pip freeze | grep -q python-dateutil==2.8.2

docker-compose-symlink:
    file.symlink:
        - name: /usr/local/bin/docker-compose
        - target: /opt/tox/bin/docker-compose
        - require:
            - cmd: install-docker-compose

/etc/docker/daemon.json:
    file.managed:
        - makedirs: True
        - source: salt://{{ slspath + '/conf/daemon.json' }}

ipv6-forwarding:
    sysctl.present:
        - name: net.ipv6.conf.all.forwarding
        - value: 1
        - config: /etc/sysctl.d/docker-forwarding.conf
        - unless:
            - "grep -q 1 /proc/sys/net/ipv6/conf/all/forwarding"

inotify-instances-increase:
    sysctl.present:
        - name: fs.inotify.max_user_instances
        - value: 8192
        - config: /etc/sysctl.d/docker-inotify.conf
        - unless:
            - "grep -q 8192 /proc/sys/fs/inotify/max_user_instances"

/etc/boto.cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/boto.cfg' }}

/etc/s3cmd.cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/s3cmd.cfg' }}

/etc/ext-s3cmd.cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/ext-s3cmd.cfg' }}

/etc/gpn-s3cmd.cfg:
    file.absent

/etc/gpn-errata-s3cmd.cfg:
    file.absent

/etc/sleet.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/sleet.json' }}

/etc/yc/cli/config.yaml:
    file.managed:
        - template: jinja
        - mode: 644
        - makedirs: True
        - source: salt://{{ slspath + '/conf/yc-config.conf' }}

/etc/yc/sa-tokens.json:
    file.managed:
        - template: jinja
        - mode: 644
        - makedirs: True
        - source: salt://{{ slspath + '/conf/sa-tokens.json' }}

/opt/yandex/CA.pem:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - require:
            - file: /opt/yandex/allCAs.pem

/etc/cron.yandex/dbaas_cleanup_hourly.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/dbaas_cleanup_hourly.sh' }}
        - mode: 755
        - require:
            - pkg: docker-pkgs

/etc/cron.yandex/dbaas_cleanup_daily.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/dbaas_cleanup_daily.sh' }}
        - mode: 755
        - require:
            - pkg: docker-pkgs

/etc/cron.d/dbaas_cleanup:
    file.managed:
        - source: salt://{{ slspath + '/conf/dbaas_cleanup.cron' }}
        - require:
            - file: /etc/cron.yandex/dbaas_cleanup_hourly.sh
            - file: /etc/cron.yandex/dbaas_cleanup_daily.sh
            - pkg: docker-pkgs

/home/robot-pgaas-ci/.docker:
    file.directory:
        - user: robot-pgaas-ci
        - group: dpt_virtual_robots_1561

/home/robot-pgaas-ci/.docker/config.json:
    file.managed:
        - source: salt://{{ slspath + '/conf/docker-robot-pgaas-ci.json' }}
        - template: jinja
        - user: robot-pgaas-ci
        - group: dpt_virtual_robots_1561
        - mode: '0600'
        - require:
            - file: /home/robot-pgaas-ci/.docker

/home/robot-pgaas-ci/.dbaas-infrastructure-test.yaml:
    file.managed:
        - source: salt://{{ slspath + '/conf/dbaas-infrastructure-test-robot-pgaas-ci.yaml' }}
        - user: robot-pgaas-ci
        - group: dpt_virtual_robots_1561
        - mode: '0600'

docker-group:
    group.present:
        - system: True
        - name: docker
        - members: {{ (salt['pillar.get']('data:docker_users', []) + ['robot-pgaas-ci']) | tojson }}
        - require:
            - pkg: docker-pkgs

hbf-agent-rules:
    file.managed:
        - name: /usr/share/yandex-hbf-agent/rules.d/20-manual.v6
        - template: jinja
        - source: salt://{{ slspath + '/conf/hbf-agent-rules' }}

/etc/security/limits.d/00-robot-pgaas-ci.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/00-robot-pgaas-ci.conf
        - mode: 644
        - user: root
        - group: root
        - makedirs: True

/usr/local/bin/pass_context.py:
    file.managed:
        - source: salt://{{ slspath + '/conf/pass_context.py' }}
        - mode: 755

/usr/local/bin/arcadia_pass_context.py:
    file.managed:
        - source: salt://{{ slspath + '/conf/arcadia_pass_context.py' }}
        - mode: 755

/usr/local/bin/docker_images_prune.py:
    file.managed:
        - source: salt://{{ slspath + '/conf/docker_images_prune.py' }}
        - mode: 755

/u0:
    mount.mounted:
        - device: /dev/md2
        - fstype: ext4
        - mkmnt: True
        - opts:
            - defaults
            - noatime
            - nodiratime
        - onlyif:
            - grep "md2" /proc/mdstat
