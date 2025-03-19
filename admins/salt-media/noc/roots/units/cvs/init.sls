{% set unit = "cvs" %}

packages:
  pkg.installed:
    - pkgs:
      - cron
      - ssh
      - rsync
      - rcs
      - apache2
      - apache2-suexec-pristine
      - libipc-run-perl
      - libmime-tools-perl
      - liburi-perl
      - enscript
      - libmime-types-perl 
      - cowsay
      - jq
      - parallel
    - skip_suggestions: True
    - install_recommends: True

/etc/fstab:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/fstab

/root/:
  file.recurse:
    - source: salt://{{ slspath }}/files/root/

bash /root/build_cvs.sh:
  cmd.run:
    - creates: /usr/bin/cvs

/etc/ssh/banner:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ssh/banner
    - template: jinja

/etc/ssh/sshd_config:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ssh/sshd_config
    - template: jinja

/etc/cron.d/rsync_from_master:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/rsync_from_master
    - template: jinja

/etc/cron.d/backup_cvs:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/backup_cvs

/usr/sbin/:
  file.recurse:
    - source: salt://{{ slspath }}/files/usr/sbin/
    - file_mode: 777
    - template: jinja

/opt/config.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/opt/config.sh
    - makedirs: True
    - mode: 777
    - template: jinja

{% for file in [
  "get_master",
  "is_master",
  "is_realserver",
  "is_slave",
] %}
/sbin/{{ file }}:
  file.managed:
    - source: salt://{{ slspath }}/files/sbin/{{ file }}
    - mode: 777
{% endfor %}

/var/www/cvs/data/:
  file.recurse:
    - source: salt://{{ slspath }}/files/var/www/cvs/data/
    - makedirs: True

/var/www/cvs/cgi-bin/:
  file.recurse:
    - source: salt://{{ slspath }}/files/var/www/cvs/cgi-bin/
    - makedirs: True
    - file_mode: 500

/var/www/cvs/icons/:
  file.recurse:
    - source: salt://{{ slspath }}/files/var/www/cvs/icons/
    - makedirs: True

/etc/rsyncd.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/rsyncd.conf

/usr/local/etc/cvsweb/:
  file.recurse:
    - source: salt://{{ slspath }}/files/usr/local/etc/cvsweb/
    - makedirs: True

/etc/apache2/sites-enabled/cvs.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/apache2/sites-enabled/cvs.conf
    - template: jinja

/etc/apache2/sites-enabled/000-default.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/apache2/sites-enabled/000-default.conf

{% for file in [
  "ssh_host_dsa_key",
  "ssh_host_dsa_key.pub",
  "ssh_host_ecdsa_key",
  "ssh_host_ecdsa_key.pub",
  "ssh_host_ed25519_key",
  "ssh_host_ed25519_key.pub",
  "ssh_host_key.pub",
  "ssh_host_rsa_key",
  "ssh_host_rsa_key.pub",
] %}
/etc/ssh/{{ file }}:
  file.managed:
    - contents_pillar: sec:{{ file }}
    - mode: 600
{% endfor %}

{% for file in [
  "dns-robot",
  "nocdeploy",
  "racktables",
] %}
/home/{{ file }}/.ssh/authorized_keys:
  file.managed:
    - contents_pillar: sec:{{ file }}
    - makedirs: True
    - user: {{ file }}
    - group: {{ file }}
    - mode: 600
{% endfor %}
