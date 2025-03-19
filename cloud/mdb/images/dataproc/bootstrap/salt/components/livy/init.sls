include:
    - .packages
    - .configs
{% if not salt['ydputils.is_presetup']() and salt['ydputils.check_roles'](['masternode'])%}
    - .services
{% endif %}
