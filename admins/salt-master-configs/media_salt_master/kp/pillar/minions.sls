salt_minion:
  lookup:
{% if grains['yandex-environment'] in ['production', 'prestable'] %}
    masters_group: kp-stable-salt
{% else %}
    masters_group: kp-test-salt
{% endif %}
