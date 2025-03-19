{% from slspath + "/map.jinja" import ftp_users with context %}

{% for dir in ftp_users.service_dirs %}
/srv/ftp/{{ dir }}:
    file.directory:
        - user: ftp
        - group: ftp
        - mode: 755
        - makedirs: True
{% endfor %}
