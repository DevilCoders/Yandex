{% set is_walg_enabled = salt.mdb_redis.is_walg_enabled() %}
include:
{% if is_walg_enabled %}
    - components.redis.walg.run-backup
{% endif %}
