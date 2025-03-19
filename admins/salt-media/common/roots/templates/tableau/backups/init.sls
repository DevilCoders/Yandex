include:
  - .setup

{% if grains.get('fqdn') == 'tableau01h.dwh.media.yandex.net' %}
include:
  - .cron
{% endif %}
