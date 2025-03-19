"""
Describes flavor- or version-dependent defaults.
"""
from .constants import (
    DEFAULT_MAX_WAL_FRAC,
    DEFAULT_MIN_WAL_FRAC,
    MAX_DEFAULT_MAINTENANCE_WORK_MEM_GB,
    MAINTENANCE_WORK_MEM_STEP_MB,
)
from ...utils.types import GIGABYTE, MEGABYTE


def get_max_connections(instance_type: dict, **_) -> int:
    """
    salt['pillar.get'](
        'data:config:max_connections',
         [100, 200 * salt['pillar.get']('data:dbaas:flavor:cpu_guarantee', 20)] | max
    )
    """
    return max(100, int(instance_type['cpu_guarantee'] * 200))


def get_shared_buffers(instance_type: dict, **_) -> int:
    """
    {% set default_shared_buffers = '8GB' %}
    {% if salt['pillar.get']('data:dbaas:flavor:memory_limit') %}
    {% set default_shared_buffers_num = (salt['pillar.get'](
           'data:dbaas:flavor:memory_limit') // 1024 // 1024 // 4)|int %}
    {% if default_shared_buffers_num > 8192 %}
    {% set default_shared_buffers = '8GB' %}
    {% else %}
    {% set default_shared_buffers = default_shared_buffers_num|string + 'MB' %}
    {% endif %}
    {% endif %}
    """
    # Convert to megabytes and get a number of pages
    shared_buffers_num = instance_type['memory_limit'] / MEGABYTE // 4
    if shared_buffers_num > 8192:
        return 8 * GIGABYTE
    return int(shared_buffers_num * MEGABYTE)


def get_max_wal_size(disk_size, **_) -> int:
    """
    Returns default max_wal_size
    {% if pg.version.major_num >= 905                                                                 %}
    {%     if salt['pillar.get']('data:dbaas:flavor')                                                 %}
    {%         set MEGABYTE = 1024 ** 2                                                               %}
    {%         set GIGABYTE = 1024 ** 3                                                               %}
    {%         set coef_GB_MB = GIGABYTE / MEGABYTE                                                   %}
    {%         set space_limit = salt['pillar.get']('data:dbaas:space_limit')                         %}
    {%         set default_min_wal_size = [ coef_GB_MB, ((space_limit * 0.05 / MEGABYTE)|int)]|min    %}
    {%         set default_max_wal_size = [8 * coef_GB_MB, ((space_limit * 0.1 / MEGABYTE)|int)]|min  %}
    {%     else                                                                                       %}
    {%         set default_min_wal_size = '1GB'                                                       %}
    {%         set default_max_wal_size = '16GB'                                                      %}
    {%     endif                                                                                      %}
    """
    return min(8 * GIGABYTE, MEGABYTE * (int(DEFAULT_MAX_WAL_FRAC * disk_size) // MEGABYTE))


def get_min_wal_size(disk_size, **_) -> int:
    """
    See get_max_wal_size() above
    """
    return min(GIGABYTE, MEGABYTE * (int(DEFAULT_MIN_WAL_FRAC * disk_size) // MEGABYTE))


def get_autovacuum_max_workers(instance_type: dict, **_) -> int:
    """
    {% if cpu > 3 %}
        {% set default_autovacuum_max_workers = [cpu, 32] | min %}
    {% else %}
        {% set default_autovacuum_max_workers = 3 %}
    {% endif %}
    """
    return min(max(3, instance_type['cpu_limit']), 32)


def get_autovacuum_vacuum_cost_delay(instance_type: dict, **_) -> int:
    """
    {% set default_autovacuum_vacuum_cost_delay = -5 * cpu + 55 %}
    {% if default_autovacuum_vacuum_cost_delay < 5 %}
        {% set default_autovacuum_vacuum_cost_delay = 5 %}
    {% endif %}
    """
    return max(5, -5 * instance_type['cpu_limit'] + 55)


def get_autovacuum_vacuum_cost_limit(instance_type: dict, **_) -> int:
    """
    {% set default_autovacuum_vacuum_cost_limit = 150 * cpu  + 400 %}
    """
    return 150 * instance_type['cpu_limit'] + 400


def get_maintenance_work_mem(instance_type: dict, **_) -> int:
    """
    {% set default_maintenance_work_mem = 64 %}
    {% if memory_limit %}
    {%  set maint_memory_free_mb = (memory_limit - default_shared_buffers_num) // 1024 // 1024 // 8 %}
    {%  set max_default_maintenance_work_mem = [ maint_memory_free_mb - (default_autovacuum_max_workers + 1) * 1024, maint_memory_free_mb // default_autovacuum_max_workers ] | max %}
    {%  set default_maintenance_work_mem = [max_default_maintenance_work_mem, 1024] | min %}
    {% endif %}
    {% set default_maintenance_work_mem = ((default_maintenance_work_mem // 64) * 64) | string + 'MB' %}
    """
    memory_limit = instance_type['memory_limit']
    shared_buffers = get_shared_buffers(instance_type)
    autovacuum_max_workers = get_autovacuum_max_workers(instance_type)

    maint_memory_free = (memory_limit - shared_buffers) / 8
    maintenance_work_mem = min(
        MAX_DEFAULT_MAINTENANCE_WORK_MEM_GB * GIGABYTE,
        max(
            maint_memory_free - (autovacuum_max_workers + 1) * GIGABYTE,
            maint_memory_free // autovacuum_max_workers,
        ),
    )
    return (maintenance_work_mem // (MAINTENANCE_WORK_MEM_STEP_MB * MEGABYTE)) * MAINTENANCE_WORK_MEM_STEP_MB * MEGABYTE


def get_pgsa_sample_interval(instance_type: dict, **_) -> int:
    """
    {% if cpu > 2 %}
        {% set interval = 1 %}
    {% else %}
        {% set interval = 60 %}
    {% endif %}
    """
    interval = 1 if instance_type['cpu_limit'] > 2 else 60
    return interval


def get_pgss_sample_interval(instance_type: dict, **_) -> int:
    """
    {% if cpu > 2 %}
        {% set interval = 60 %}
    {% else %}
        {% set interval = 600 %}
    {% endif %}
    """
    interval = 60 if instance_type['cpu_limit'] > 2 else 600
    return interval
