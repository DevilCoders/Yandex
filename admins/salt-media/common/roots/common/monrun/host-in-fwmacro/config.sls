{% set conf = pillar.get("host_in_macro_check") %}
{% if conf %}
/etc/monitoring/host-in-macro-check.yaml:
  file.managed:
    - makedirs: True
      contents: |
        {{ conf|yaml(False)|indent(8) }}
{% endif %}

