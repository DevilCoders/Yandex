{% set services = salt['pillar.get']('data:services', []) %}
{% if salt['ydputils.check_roles'](['masternode']) and services != [] %}
include:
    - .packages
{% if not salt['ydputils.is_presetup']() %}
    - .configs
    - .secrets
{% endif %}
{% endif %}
