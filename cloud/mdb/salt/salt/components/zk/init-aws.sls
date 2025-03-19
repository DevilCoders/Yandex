{% set dbaas = salt.pillar.get('data:dbaas', {}) %}
{% set working_with_kafka = salt.pillar.get('data:zk:apparmor_disabled', False) %}

include:
   - .main
   - .systemd
   - components.logrotate
{% if dbaas %}
   - .resize
{% endif %}
{% if not dbaas.get('cloud') %}
   - components.firewall
   - components.firewall.external_access_config
{% endif %}
{% if not working_with_kafka %}
   - .firewall
{% endif %}
{% if salt.pillar.get('data:unmanaged:enable_zk_tls', False) %}
   - .zkpass
{% endif %}
   - .telegraf
{% if salt.pillar.get('service-restart', False) %}
   - .restart
{% endif %}