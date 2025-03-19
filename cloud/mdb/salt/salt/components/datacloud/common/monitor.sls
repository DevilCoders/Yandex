monitor-group:
    group.present:
        - name: monitor
        - system: True

monitor-user:
    user.present:
        - fullname: System monitoring user
        - name: monitor
{% if salt['grains.get']('saltversioninfo')[0] >= 3001 %}
        - usergroup: True
{% else %}
        - gid_from_name: True
{% endif %}
        - empty_password: False
        - shell: /bin/false
        - system: True
        - require:
            - group: monitor
