include:
    - .packages
{% if not salt['ydputils.is_presetup']() and salt['ydputils.check_roles'](['masternode', 'datanode']) %}
    - .configs
    - .services
    - .hdfs-directories
{% endif %}
