{# Do not deploy kimimr role if external kikimr_endpoint defined #}
{% if salt['grains.get']('overrides:kikimr_endpoint') == '' %}
include:
  - kikimr
{% endif %}
