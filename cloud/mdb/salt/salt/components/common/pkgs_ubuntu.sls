{% set osrelease = salt['grains.get']('osrelease') %}
{% set deploy_version = salt['pillar.get']('data:deploy:version', 1) %}
{% set deploy_api_host = salt['pillar.get']('data:deploy:api_host', '') %}
{% set env = salt['pillar.get']('yandex:environment', 'prod') %}
{% set salt_masterless = salt['pillar.get']('data:salt_masterless', False) %}
{% load_yaml as pkgs %}
apt: any
bash-completion: any
python-msgpack: 3-4fc9b70
python-boto: any
python-botocore: any
python-concurrent.futures: any
python-setproctitle: any
mdb-ssh-keys: '163-b210281'
{% if not salt_masterless %}
mdb-config-salt: '1.9267384-trunk'
{% endif %}
config-ssh-banner: any
config-yabs-ntp: any
curl: any
dstat: any
file: any
bind9-host: any
gdb: any
htop: any
iftop: any
iotop: any
iptables: any
less: any
lsof: any
lsscsi: any
mailutils: any
man-db: any
mdb-juggler-tools: '1.9056841'
netcat-openbsd: any
ntpdate: any
postfix: any
psmisc: any
pv: any
retry: "'1.6101280'"
rsync: any
rsyslog: any
screen: any
ssh: any
strace: any
sudo: any
sysstat: any
tcpdump: any
telnet: any
tmux: any
traceroute: any
vim: any
virt-what: any
vlan: any
wget: any
whois: any
yandex-archive-keyring: any
yandex-dash2bash: any
zsh: any
yandex-search-user-monitor: '1.0-45'
python3-zmq: '17.1.2-3ubuntu1+yandex0'
{% endload %}

{% if salt['pillar.get']('data:allow_salt_version_update', True) %}
    {% set salt_version = salt['pillar.get']('data:salt_version', '3000.9+ds-1+yandex0') %}
    {% do pkgs.update({
        'salt-minion': salt_version,
        'salt-common': salt_version,
    }) %}
{% endif %}

yamail-ssh-keys:
    pkg.purged:
        - require_in:
            - pkg: common-packages
yamail-config-salt:
    pkg.purged:
        - require_in:
            - pkg: common-packages

missing-authorized-keys2-workaround:
    pkg.purged:
        - name: mdb-ssh-keys
        - require_in:
            - pkg: common-packages
        - unless:
            - ls /root/.ssh/authorized_keys2

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
    {% set hwhosts_pkgs = {
        'ethtool': 'any',
        'gdisk': 'any',
        'ipmitool': 'any',
        'irqbalance': 'any',
        'smartmontools': 'any',
        'libc6': 'any',
        }
    %}

{%     if 'KVM' not in salt['grains.get']('cpu_model') and salt['grains.get']('virtual', 'physical') != 'kvm' %}
{%         do hwhosts_pkgs.update({'grub-nvme': '2.04-3', 'mcelog': 'any'}) %}
{%     else %}
{%         do hwhosts_pkgs.update({'grub2': 'any'}) %}
{%     endif %}

    {% do pkgs.update(hwhosts_pkgs) %}
{% endif %}

{% if not salt['pillar.get']('data:use_unbound_64', False) %}
    {% set bind9_pkgs = {
        'bind9': 'any'
        }
    %}

    {% do pkgs.update(bind9_pkgs) %}
{% endif %}

/etc/debconf.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/debconf.conf
        - require_in:
            - cmd: repositories-ready

common-packages:
    pkg.installed:
        - pkgs:
{% for package, version in pkgs.items() %}
{% if version is mapping %}
{%   set version = version[osrelease] %}
{% endif %}
{% if version == 'any' %}
            - {{ package }}
{% else %}
            - {{ package }}: {{ version }}
{% endif %}
{% endfor %}
        - prereq_in:
            - cmd: repositories-ready
        - require:
            - pkg: remove-skynet-simple-single-bundle
{% if osrelease == '18.04' %}
            - pkgrepo: mdb-bionic-stable-all
            - pkgrepo: mdb-bionic-stable-arch
{% endif %}
{% if salt['pillar.get']('data:use_unbound_64', False) %}
            - pkg: unbound-config-local64
{% else %}
{% if salt['pillar.get']('data:dbaas:vtype') != 'compute' %}
            - file: /etc/resolv.conf
{% endif %}
{% endif %}
        # workaround: override allCAs from mdb-config-salt
        - require_in:
            - file: /opt/yandex/allCAs.pem

config-autodetect-active-eth:
    pkg.purged

remove-skynet-versioned-metapackage:
    pkg.purged:
        - name: yandex-skynet-releaseconf

remove-skynet-simple-single-bundle:
    pkg.purged:
        - name: skynet-simple-single-bundle

/etc/salt/minion.d/zz_debug.conf:
    file.absent

/etc/salt/minion.d/zz_error.conf:
{% if not (env in ('dev', 'qa') or salt.pillar.get('data:salt_minion:debug')) %}
    file.managed:
        - contents: |
            log_level_logfile: error
        - user: root
        - group: root
        - mode: '0644'
{% else %}
    file.absent
{% endif %}

/etc/salt/minion.d/s3.conf:
{% if salt.pillar.get('data:salt_minion:configure_s3') %}
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/salt/minion.d/s3.conf
        - template: jinja
        - user: root
        - group: root
        - mode: '0644'
{% else %}
    file.absent
{% endif %}

/etc/salt/minion.d/aws.conf:
{% if salt['dbaas.pillar']('data:dbaas:vtype', 'unknown') == 'aws' %}
    file.managed:
        -   source: salt://{{ slspath }}/conf/etc/salt/minion.d/aws.conf
        -   template: jinja
        -   user: root
        -   group: root
        -   mode: '0644'
{% else %}
    file.absent
{% endif %}


/etc/salt/minion.d/zz_saltenv.conf:
    file.absent

/etc/salt/minion.d/yandex.saltenv.conf:
    file.absent

{% if salt['pillar.get']('data:conductor_host') %}
# don't add file.absent branch, cause we have custom conductor_host (proxy in that case) only in private-regions.
/etc/salt/minion.d/zz_conductor.conf:
    file.managed:
        - user: root
        - group: root
        - mode: '0644'
        - contents: "conductor_host: {{ salt['pillar.get']('data:conductor_host') }}"
{% endif %}

/etc/yandex/mdb-deploy:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /etc/yandex

/etc/yandex/mdb-deploy/deploy_version:
    file.managed:
        - user: root
        - group: root
        - mode: '0644'
        - makedirs: True
        - replace: False
        - contents: |
            {{ deploy_version }}
        - require:
            - file: /etc/yandex/mdb-deploy

{% if deploy_api_host != '' %}
/etc/yandex/mdb-deploy/mdb_deploy_api_host:
    file.managed:
        - user: root
        - group: root
        - mode: '0640'
        - makedirs: True
        - contents: |
            {{ deploy_api_host }}
        - require:
            - file: /etc/yandex/mdb-deploy
{% endif %}

/etc/yandex/mdb-deploy/job_result_blacklist.yaml:
    file.managed:
        - user: root
        - group: root
        - mode: '0644'
        - makedirs: True
        - source: salt://{{ slspath }}/conf/etc/yandex/mdb-deploy/job_result_blacklist.yaml
        - require:
            - file: /etc/yandex/mdb-deploy

# For now set this only for non-bionic machines (there are no systemd units)
include:
{% if not salt_masterless %}
    - components.deploy.mdb-ping-salt-master
{% endif %}
    - components.grpcurl
