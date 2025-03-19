{% if grains.get("lsb_distrib_release", "").startswith("10.") %}
timeout:
  pkg:
    - installed
{% endif %}
