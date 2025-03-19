{% if salt.mdb_mongodb_helpers.services_deployed() != ['mongos'] %}
include:
{% for srv in salt.mdb_mongodb_helpers.services_deployed() if srv != 'mongos' %}
    - .{{ srv }}-stepdown-host
{% endfor %}
{% endif %}
