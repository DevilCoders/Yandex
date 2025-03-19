{% from slspath + "/map.jinja" import ftp_users with context %}


sshd_config:
    file.managed:
        - name: /etc/ssh/sshd_config
        - source: salt://{{ slspath }}/files/ssh/sshd_config
        - user: root
        - group: root
        - mode: 600
        - template: jinja
        - context:
          ftp_users: {{ ftp_users }}

ssh:
  service.running:
    - enable: True
    - watch:
      - file: sshd_config

{% for user in ftp_users.sftp %}

{{ user }}:
    user.present:
    - shell: /bin/false
    - home: /srv/ftp/{{ user }}
    - groups:
        - ftp
    - require:
        - pkg: vsftpd

{{ user }}_ssh_key:
  ssh_auth.present:
    - user: {{ user }}
    - source: salt://{{ slspath }}/files/ssh/keys/{{ user }}.id_rsa.pub
    - require:
        - user: {{ user }}

/srv/ftp/{{ user }}:
    file.directory:
        - user: root
        - group: ftp
        - mode: 755
        - require:
            - pkg: vsftpd

  {% for dir in ftp_users.sftp_dirs %}
/srv/ftp/{{ user }}/{{ dir }}:
    file.directory:
        - user: {{ user }}
        - group: {{ user }}
        - mode: 770
        - require:
            - user: {{ user }}
  {% endfor %}

  {% if user in "sony_sftp" %}
/srv/ftp/{{ user }}/test/data:
    file.directory:
        - makedir: True
        - user: {{ user }}
        - group: {{ user }}
        - mode: 770
        - require:
            - user: {{ user }}
            - file: /srv/ftp/{{ user }}

/srv/ftp/{{ user }}/test/logs:
    file.directory:
        - makedir: True
        - user: {{ user }}
        - group: {{ user }}
        - mode: 770
        - require:
            - user: {{ user }}
            - file: /srv/ftp/{{ user }}
  {% endif %}
{% endfor %}


