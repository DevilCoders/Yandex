{% if grains['conductor']['project'] == 'ape' %}
{{ pillar.get('timezone', 'Etc/UTC') }}:
  timezone.system
{%- else -%}
{{ pillar.get('timezone', 'Europe/Moscow') }}:
  timezone.system
{% endif %}
