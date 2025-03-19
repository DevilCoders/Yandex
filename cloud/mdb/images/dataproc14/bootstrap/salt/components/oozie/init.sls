include:
    - .packages
{% if not salt['ydputils.is_presetup']() and salt['ydputils.check_roles'](['masternode']) %}
    - .postgres
    - .configs
    - .hdfs
    - .services
{% endif %}
