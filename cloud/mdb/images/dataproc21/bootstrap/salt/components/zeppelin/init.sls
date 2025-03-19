include:
    - .packages
{% if not salt['ydputils.is_presetup']() and salt['ydputils.is_masternode']() %}
    - .configs
    - .services
{% endif %}
