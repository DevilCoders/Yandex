include:
    - .packages
{% if not salt['ydputils.is_presetup']() and salt['ydputils.check_roles'](['masternode']) %}
    - .drivers
    - .postgres
    - .configs
    - .metastore
    - .services
{% endif %}
