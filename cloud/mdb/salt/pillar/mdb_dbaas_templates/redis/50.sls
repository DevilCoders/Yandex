{% from "map.jinja" import env_and_major_version_2_redis_pkg with context %}
{% from "mdb_dbaas_templates/init.sls" import env with context %}

include:
    - mdb_dbaas_templates.redis.base

data:
{% set redis_major_version = '5.0' %}
    versions:
        redis:
            major_version: "{{ redis_major_version }}"
            package_version: {{ env_and_major_version_2_redis_pkg[(env, redis_major_version, 'default')] }}
