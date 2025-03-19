service-postgresql:
{% if salt['ydputils.check_roles'](['masternode']) and not salt['ydputils.is_presetup']() %}
    service.running:
        - enable: true
{% else %}
    service.disabled:
{% endif %}
        - name: postgresql
        - require:
            - pkg: postgres_packages
        - watch:
            - pkg: postgres_packages
