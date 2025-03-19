{%- from "common/push-client/map.jinja" import push_client with context -%}

DAEMON_CONF=""
{% for name, instance in push_client.instances.items() %}
{% if instance.get('enabled', False) and instance.get('files', []) %}
CONF_PATH="/etc/yandex/statbox-push-client/conf.d/push-client-{{name}}.yaml"
DAEMON_CONF+=" ${CONF_PATH}"
{% endif %}
{% endfor %}
