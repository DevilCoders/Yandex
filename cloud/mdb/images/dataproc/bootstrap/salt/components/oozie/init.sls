include:
    - .packages
{% if salt['ydputils.check_roles'](['masternode']) %}
    - .postgres
{% if not salt['ydputils.is_presetup']() %}
    - .configs
    - .hdfs
    - .services
{% endif %}
{% endif %}
