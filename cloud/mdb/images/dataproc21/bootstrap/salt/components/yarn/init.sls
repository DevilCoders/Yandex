include:
    - .packages
{% if not salt['ydputils.is_presetup']() %}
    - .configs
    - .services
{% endif %}
