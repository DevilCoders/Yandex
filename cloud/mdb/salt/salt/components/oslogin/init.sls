include:
{% if salt.pillar.get('data:oslogin:enabled', False) %}
  - .pkgs
  - .main
  - .breakglass
{% else %}
  - .disable
{% endif %}
