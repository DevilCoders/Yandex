{% set unit = "racktables" %}

include:
  - .rsyslog-configs
  {% if grains["kernel"] == "Linux" %}
  # Временно эти стейты НЕ включены для FreeBSD.
  # Каждый стейт нужно отдельно проверить на noc-sas, noc-myt и аккуратно включить.
  # На первом этапе подключается только конфига rsyslog
  - templates.certificates
  - .home
  - .mondata
  - .php
  - .logstat
  - .python
  {% elif grains["kernel"] == "FreeBSD" %}
  - .bsd
  {% endif %}
