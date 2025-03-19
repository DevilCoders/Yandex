{% set oscodename = grains['oscodename'] %}
{% set init = grains['init'] %}

vnc4server:
  pkg.removed

yabro_repo_key:
  cmd.run:
    - name: curl -s https://repo.yandex.ru/yandex-browser/YANDEX-BROWSER-KEY.GPG -o /tmp/yandex-browser-key.gpg | apt-key add /tmp/yandex-browser-key.gpg
    - unless: dpkg -l | grep yandex-browser-beta

yabro_repo:
  pkgrepo.managed:
    - humanname: "repo for yandex-browser-beta"
    - name: deb http://repo.yandex.ru/yandex-browser/deb beta main
    - file: /etc/apt/sources.list.d/yandex-browser-beta.list
    - require:
      - cmd: yabro_repo_key

vnc_packages:
  pkg.installed:
    - pkgs:
      {% if oscodename == 'trusty' %}
      - autocutsel
      - libfontenc1
      - libfontenc1
      - libgnutls28
      - libjpeg-turbo8
      - libpixman-1-0
      - libtasn1-3-bin
      - libxfont1
      - libxtst6
      - libglu1-mesa
      - x11-xkb-utils
      {% endif %}
      - xfce4-terminal
      - yandex-browser-beta
      - firefox
      - filezilla
      - xfce4

{% if init == 'systemd' %}
/etc/systemd/system/vncserver.service:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/systemd/system/vncserver.service
    - makedir: True

service.systemctl_reload:
  module.run:
    - onchanges:
      - file: /etc/systemd/system/vncserver.service

{% endif %}

/usr/local/bin/vncserver_init.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/vncserver_init.sh
    - makedir: True
    - mode: 0755

{% if oscodename == 'trusty' %}
get_tigervncserver:
  cmd.run:
    - name: wget -P /tmp/ 'https://dl.bintray.com/tigervnc/stable/ubuntu-14.04LTS/amd64/tigervncserver_1.6.0-3ubuntu1_amd64.deb'
    - require:
      - pkg: vnc4server
    - watch_in:
      - cmd: install_tigervncserver
    - unless: dpkg -l | grep tigervncserver

install_tigervncserver:
  cmd.wait:
    - name: dpkg -i /tmp/tigervncserver_1.6.0-3ubuntu1_amd64.deb || true; apt-get -f install -y --force-yes
    - unless: dpkg -l | grep tigervncserver

/etc/init.d/vncserver:
  file.symlink:
    - target: /usr/local/bin/vncserver_init.sh
    - force: True
{% else %}
tigervnc-standalone-server:
  pkg.installed
{% endif %}

/etc/yandex/config4vnc.default:
  file.managed:
    - source: salt://{{slspath}}/files/config
    - makedir: True
    - mode: 0644

/etc/yandex/xstartup.default:
  file.managed:
    - source: salt://{{slspath}}/files/xstartup.tmpl
    - makedir: True
    - mode: 0755

/etc/default/vncservers:
  file.managed:
    - source: salt://{{slspath}}/files/vncservers_default
    - template: jinja
    - mode: 0644
    - require:
      - pkg: vnc_packages
      {% if oscodename == 'trusty' %}
      - cmd: install_tigervncserver
      {% else %}
      - pkg: tigervnc-standalone-server
      {% endif %}
    - context:
        users_string: {{ salt["pillar.get"]("vncserver:users_string") }}

/etc/yandex/vnc_config_users.sh:
  file.managed:
    - source: salt://{{slspath}}/files/user_config.tmpl
    - template: jinja
    - mode: 755
    - context:
        users: {{ salt["pillar.get"]("vncserver:users") }}

config_users:
  cmd.run:
    - name: /etc/yandex/vnc_config_users.sh

vncserver:
  cmd.run:
    {% if init == 'systemd' %}
    - name: systemctl restart vncserver
    {% else %}
    - name: /etc/init.d/vncserver restart
    {% endif %}

# create users config:
    # su $USER
    # vncserver (specify password and creates .Xautority)
    # cp /etc/yandex/xstartup.default /home/$USER/.vnc/xstartup
    # cp /etc/yandex/config4vnc.default /home/$USER/.vnc/config
# if support will be required user config should be done automatically
# when host in CADMIN-6163 will not needed, this sls must be removed
