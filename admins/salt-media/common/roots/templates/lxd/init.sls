{% from slspath + '/map.jinja' import lxd with context %}
{% set lxc_binary = "/snap/bin/lxc" if lxd.base.snap.enabled else "/usr/bin/lxc" %}

# Repo and common lxd settings
include:
{% if lxd.base.latest and not lxd.base.snap.enabled %}
  - .repo
{%- endif %}
  - .service
{% if lxd.init.backend == 'zfs' %}
  - templates.zfs
{% endif %}

# Yandex specific settings
yandex_lxd:
  file.directory:
    - name: /etc/yandex/lxd
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

{% if lxd.projects %}
{% for project in lxd.projects %}
{% if project.get('name') %}
yandex_lxd_{{ project.name }}_profile:
  file.managed:
    - name: /etc/yandex/lxd/profiles/{{ project.name }}
    - source:
      {% if lxd.profiles %}
      - {{ lxd.profiles }}/{{ project.name }}
      {% endif %}
      {% if lxd.base_profiles %}
      - {{ lxd.base_profiles }}/{{ project.name }}
      {% endif %}
      {% if lxd.dynamic_profiles %}
      - {{ lxd.dynamic_profiles }}
      {% endif %}
    - makedirs: True
    - user: root
    - group: root
    - mode: 0600
    - dir_mode: 0755
    - template: jinja
    - context:
        ctx: {{ project }}
    - require:
      - pkg: lxd_service
{% endif %}
{% endfor %}
{% endif %}

# Fix subgid/subuid issue
Check subgid,subuid:
  cmd.run:
    - name: sed -i 's/^\([lxd,root].*\):[0-9]\+$/\1:624288/g' /etc/subgid /etc/subuid
    - unless: egrep 'lxd:[0-9]+:624288' /etc/subgid && egrep 'lxd:[0-9]+:624288' /etc/subuid && egrep 'root:[0-9]+:624288' /etc/subuid && egrep 'root:[0-9]+:624288' /etc/subgid

{%- if lxd.base.snap.enabled %}
Migrate lxd to snap lxd:
  cmd.run:
    - name: |
        /snap/bin/lxd init --auto --network-address "{{ lxd.init.address }}" \
          --network-port {{ lxd.init.port }} \
          --storage-backend {{ lxd.init.backend }} --storage-pool {{ lxd.init.pool }} \
          --trust-password {{ lxd.web_pass }};
        /snap/bin/lxd.migrate -yes;
        /snap/bin/lxc config set core.https_address="{{lxd.init.address}}:{{lxd.init.port}}" core.trust_password="{{lxd.init.port}}"
    - unless: test -f /etc/yandex/lxd/salt-configured.flag
{%- endif %}

# Init lxd only if /etc/yandex/lxd/salt-configured.flag not exist
Initialize {{ lxd.init.backend }} pool:
  cmd.run:
    - name: {{lxc_binary}} storage create lxd {{ lxd.init.backend }} source=/dev/md1 {{ lxd.init.backend_opts }}
    - unless: test -f /etc/yandex/lxd/salt-configured.flag || {{lxc_binary}} storage list|grep lxd > /dev/null
    - require:
      - file: /etc/yandex/lxd

Set default volume size:
  cmd.run:
    - name: {{lxc_binary}} storage set lxd volume.size {{ lxd.init.default_volume_size }}
    - unless: test -f /etc/yandex/lxd/salt-configured.flag
    - require:
      - file: /etc/yandex/lxd

Set default mount options:
  cmd.run:
    - name: {{lxc_binary}} storage set lxd volume.block.mount_options "nobarrier,noatime"
    - unless: test -f /etc/yandex/lxd/salt-configured.flag
    - require:
      - file: /etc/yandex/lxd

Initialize {{ lxd.init.backend }} ssd pool:
  cmd.run:
    - name: {{lxc_binary}} storage create lxd-ssd {{ lxd.init.backend }} source=/dev/md2 {{ lxd.init.backend_opts }}
    - onlyif: test -b /dev/md2 && test ! -f /etc/yandex/lxd/salt-configured.flag
    - require:
      - file: /etc/yandex/lxd

Set default lxd-ssd mount options:
  cmd.run:
    - name: {{lxc_binary}} storage set lxd-ssd volume.block.mount_options "nobarrier,noatime,discard"
    - onlyif: test -b /dev/md2 && test ! -f /etc/yandex/lxd/salt-configured.flag
    - require:
      - file: /etc/yandex/lxd

{% if lxd.init.backend == 'zfs' %}
zfs lxd pool settings:
  cmd.run:
    - name: zfs set compression=off lxd; zfs set dedup=off lxd;
    - onlyif: zfs list lxd &>/dev/null && test ! -f /etc/yandex/lxd/salt-configured.flag

zfs lxd-ssd pool settings:
  cmd.run:
    - name: zfs set compression=off lxd-ssd; zfs set dedup=off lxd-ssd;
    - onlyif: zfs list lxd-ssd &>/dev/null && test -b /dev/md2 && test ! -f /etc/yandex/lxd/salt-configured.flag
{% endif %}

Initialize lxd daemon:
  cmd.run:
    - name: >
        lxd init --auto --network-address "{{ lxd.init.address }}" --network-port {{ lxd.init.port }}
        --trust-password {{ lxd.web_pass }}
        && touch /etc/yandex/lxd/salt-configured.flag
{%- if lxd.base.snap.enabled %}
        || /snap/bin/lxc config set core.https_address="{{lxd.init.address}}:{{lxd.init.port}}" core.trust_password="{{lxd.init.port}}"
        && touch /etc/yandex/lxd/salt-configured.flag
{%- endif %}
    - unless: > 
        test -f /etc/yandex/lxd/salt-configured.flag || 
        ({{lxc_binary}} profile list|grep default > /dev/null && ! {{lxc_binary}} profile show default| grep 'devices: {}' > /dev/null)
    - require:
      - file: /etc/yandex/lxd

Regular dom0hostname update:
  file.managed:
    - name: /etc/cron.d/update_dom0hostname
    - makedirs: True
    - user: root
    - group: root
    - mode: 0644
    - contents: |
        SHELL=/bin/bash
        PATH=/bin:/sbin:/usr/bin:/usr/sbin:/snap/bin
        * * * * * root lxc ls --format=csv -cn|xargs -n1 -I_ lxc file push /etc/hostname _/etc/dom0hostname &>/dev/null

# Util for correct updating of LXD profiles
lxd-profile-updater:
  file.managed:
    - name: /usr/local/bin/lxd-profile-updater
    - source: salt://{{ slspath }}/files/usr/local/bin/lxd-profile-updater
    - template: jinja
    - context:
        lxc_binary: {{ lxc_binary }}
    - mode: 0755
    - user: root
    - group: root
/var/lib/lxd/backup:
  file.directory:
    - user: lxd
    - group: nogroup
    - mode: 755
# Update profiles in lxd db if any file was changed
Update lxd profiles:
  cmd.run:
    - name: set -e ; find /etc/yandex/lxd/profiles -type f | while read conffile; do name=$(echo $conffile | awk -F\/ '{print $NF}');lxc_name=$({{lxc_binary}} profile list | awk -v name=$name '$2==name {print $2}'); echo "$name:"; /usr/local/bin/lxd-profile-updater --rename $name $conffile; done
    - onchanges:
      {% if lxd.projects %}
      {% for project in lxd.projects %}
      {% if project.name %}
      - file: yandex_lxd_{{ project.name }}_profile
      {% endif %}
      {% endfor %}
      {% endif %}


{% if lxd.images %}
{% for image_name, image_url in lxd.images.items() %}
Get {{ image_name }} image:
  cmd.run:
    - name: wget --no-verbose {{ image_url }} -O /tmp/{{ image_name }} && {{lxc_binary}} image import /tmp/{{ image_name }} --alias {{ image_name }} && rm -f /tmp/{{ image_name }}
    - unless: {{lxc_binary}} image info {{ image_name }}
{% endfor %}
{% endif %}
