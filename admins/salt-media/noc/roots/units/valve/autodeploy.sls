{% from "units/percent.sls" import packages %}

valve-packages:
  pkg.installed:
    - pkgs:
      - python3-psycopg2
      - python3-requests
      - python3-dateutil
      {% for pkg in packages %}
      - {{pkg.name}}: {{pkg.version}}
      {% endfor %}
