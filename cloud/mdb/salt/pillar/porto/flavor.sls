# Fill dbaas like flavor from porto_resources grains
{% set container = salt.grains.get('porto_resources:container', {}) %}
{% if container %}
data:
    dbaas:
        flavor:
            # L[-1] remove trailing c suffix from CPU limits
            cpu_guarantee: {{ container.cpu_guarantee[:-1] }}
            cpu_limit: {{ container.cpu_limit[:-1] }}
            memory_guarantee: {{ container.memory_guarantee }}
            memory_limit: {{ container.memory_limit }}
{% else %}
# Container limits are not available, probably python-portopy not installed
{% endif %}
