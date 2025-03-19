{% macro restart_if_needed(scenario) %}
include:
  - .common

restart-{{ scenario }}:
  cmd.run:
    - name: /usr/local/bin/safe-restart {{ scenario }}
    - stateful: true
    - require:
      - file: /usr/local/bin/safe-restart
{% endmacro %}
