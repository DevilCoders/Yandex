{% if salt['ydputils.check_roles'](['masternode']) %}
service-postgresql:
    service:
        - running
        - enable: true
        - name: postgresql
        - require:
            - pkg: postgres_packages
        - watch:
            - pkg: postgres_packages
{% endif %}
