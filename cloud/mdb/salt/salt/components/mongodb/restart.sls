{% set service_for_restart = salt.pillar.get('service-for-restart', None) %}

include:
{% for srv in salt.mdb_mongodb_helpers.services_deployed() if service_for_restart in [srv, None] %}
    - .{{ srv }}-restart
{% endfor %}
