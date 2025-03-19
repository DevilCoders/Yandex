{% set cgroups = grains['conductor']['groups'] %}

s3pkgs:
  pkg.installed:
    - pkgs:
      - jq
      # - libzstd: 1.2.3
      {% if 'elliptics-test-proxies' in cgroups %}
      - postgresql-client-10
      - postgresql-client-common
      - pgdg-keyring
      - libreadline6
      - libpq5
      {% endif %}
    - skip_suggestions: True
    - install_recommends: False
