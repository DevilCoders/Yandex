include:
    - .packages
{% if salt['ydputils.check_roles'](['masternode']) %}
    - .postgres
{% endif %}
    - .drivers
{% if not salt['ydputils.is_presetup']() and salt['ydputils.check_roles'](['masternode']) %}
    - .configs
    - .metastore
    - .services
{% endif %}
