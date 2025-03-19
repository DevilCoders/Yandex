"""
Flavor-dependent defaults.
"""

# ALARMA! Do not forget to change defaults in my.cnf:
# https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/salt/salt/components/mysql/conf/my.cnf


def get_max_connections(instance_type: dict, **_) -> int:
    return max(100, int(instance_type['memory_guarantee'] / 33554432))


def get_thread_cache_size(instance_type: dict, **_) -> int:
    return int(get_max_connections(instance_type) / 10)


def get_innodb_buffer_pool_size(instance_type: dict, **_) -> int:
    memory_guarantee = instance_type['memory_guarantee']
    if memory_guarantee >= 8 * 1024**3:
        return int(0.5 * memory_guarantee)
    if memory_guarantee >= 4 * 1024**3:
        return int(0.375 * memory_guarantee)
    return int(0.125 * memory_guarantee)


def get_my_sessions_sample_interval(instance_type: dict, **_) -> int:
    """
    {% if cpu > 2 %}
        {% set default_interval = 20 %}
    {% else %}
        {% set default_interval = 60 %}
    {% endif %}
    """
    interval = 20 if instance_type['cpu_limit'] > 2 else 60
    return interval


def get_my_statements_sample_interval(instance_type: dict, **_) -> int:
    """
    {% if cpu > 2 %}
        {% set default_interval = 60 %}
    {% else %}
        {% set default_interval = 600 %}
    {% endif %}
    """
    interval = 60 if instance_type['cpu_limit'] > 2 else 600
    return interval
