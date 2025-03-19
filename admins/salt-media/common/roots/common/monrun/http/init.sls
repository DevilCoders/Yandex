{% if "http_check" in pillar %}
include:
  - .check
  - .config
  - .monrun
  - .packages
{% else %}
http_check pillar NOT DEFINED:
  cmd.run:
    - name: echo "This formula has not defaults, defined pillar are required";exit 1
{% endif %}
