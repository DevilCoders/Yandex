{% if salt['ydputils.check_roles'](['masternode']) %}
include:
    - .packages
{% if not salt['ydputils.is_presetup']() %}
    - .configs
    - .secrets
{% endif %}
{% endif %}
