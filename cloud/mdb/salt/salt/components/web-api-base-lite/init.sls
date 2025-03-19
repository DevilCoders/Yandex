
web-api-group:
    group.present:
        - name: web-api
        - system: True

web-api-user:
    user.present:
        - fullname: HTTP API system user
        - name: web-api
{% if salt['grains.get']('saltversioninfo')[0] >= 3001 %}
        - usergroup: False
{% else %}
        - gid_from_name: False
{% endif %}
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True
        - groups:
            - www-data
            - web-api
        - require:
            - group: web-api

/home/web-api/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - user: web-api
        - group: web-api
        - makedirs: True
        - require:
            - user: web-api-user

/root/.postgresql/root.crt:
    file.symlink:
        - target: /opt/yandex/allCAs.pem
        - force: True
        - user: root
        - group: root
        - makedirs: True
