{% from slspath + "/map.jinja" import ftp_users with context %}
{% set oscodename = grains['oscodename'] %}

include:
    - .xinetd


vsftpd:
  pkg.installed:
  {% if oscodename == 'trusty' %}
    - name: vsftpd
    - version: 2.2.2-3ubuntu6
    - sources:
        - vsftpd: salt://{{ slspath }}/files/deb/vsftpd_2.2.2-3ubuntu6_amd64.deb
    - require:
        - pkg: required_pkgs
        - pkg: xinetd
  {% else %}
    - require:
        - pkg: xinetd
  {% endif %}

vsftpd_config:
  file.managed:
    - name: /etc/vsftpd.conf
    - template: jinja
    - source: salt://{{ slspath }}/files/etc/vsftpd.conf
    - context:
      oscodename: {{oscodename}}
    - user: root
    - mode: 644

{% for user in ftp_users.vsftpd %}
/srv/ftp/{{ user }}:
    file.directory:
        - user: ftp
        - group: ftp
        - mode: 755
        - makedirs: True
        - require:
          - pkg: vsftpd

/etc/vsftpd/users/{{ user }}:
    file.managed:
        - user: root
        - group: root
        - mode: 644
        - source: 
          - salt://{{ slspath }}/files/etc/vsftpd/users/{{ user }}
          - salt://{{ slspath }}/files/etc/vsftpd/users/default_perms.conf
        - makedirs: True
{% endfor %}

{% for user in ftp_users.vsftpd_admin %}
/srv/ftp/{{ user }}:
    file.directory:
        - user: ftp
        - group: ftp
        - mode: 755
        - makedirs: True
        - require:
          - pkg: vsftpd

/var/tmp/vsftpd:
    file.directory:
        - user: ftp
        - group: ftp
        - mode: 555
        - makedirs: True
        - require:
          - pkg: vsftpd

/etc/vsftpd/users/{{ user }}:
    file.managed:
        - user: root
        - group: root
        - mode: 644
        - source: salt://{{ slspath }}/files/etc/vsftpd/users/{{ user }}
        - makedirs: True
{% endfor %}

{% for file in ftp_users.gen_files %}
{{ file }}:
    file.managed:
        - user: root
        - source: salt://{{ slspath }}/files{{ file }}
        - group: root
        - mode: 644
{% endfor %}

required_pkgs:
    pkg.installed:
        - pkgs:
            - config-juggler-client-media
            - juggler-client
            - libpam-pwdfile
            - config-monrun-daemon-check
            - apache2-utils
            {% if oscodename == 'trusty' %}
            - libssl0.9.8
            {% else %}
            - libssl1.0.0
            {% endif %}
            - config-monitoring-common

/etc/vsftpd/passwd:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents: {{ salt['pillar.get']('vsftpd:passwd') | json }}

