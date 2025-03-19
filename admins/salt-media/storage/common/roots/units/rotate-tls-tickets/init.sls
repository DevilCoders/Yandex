{% set secret_ids = pillar.get('tls_secret_ids') %}
{% set oauth = salt.config.get('yav.config') %}

{% for secret in secret_ids %}
yav tls rotate {{secret}} --oauth {{oauth['oauth-token']}}:
  cron.present:
    - user: {{ pillar.get('yav_user', 'robot-storage-duty') }}
    - minute: random
{% endfor %}
