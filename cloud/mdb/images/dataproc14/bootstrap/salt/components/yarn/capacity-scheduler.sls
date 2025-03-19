{% set config_site = {} %}

{% do config_site.update({'capacity-scheduler': salt['grains.filter_by']({
    'Debian': {
        'yarn.scheduler.minimum-allocation-mb': salt['ydputils.get_yarn_sched_min_allocation_mb'](),
        'yarn.scheduler.maximum-allocation-mb': salt['ydputils.get_yarn_sched_max_allocation_mb'](),
        'yarn.scheduler.maximum-allocation-vcores': salt['ydputils.get_yarn_sched_max_allocation_vcores'](),
        'yarn.scheduler.capacity.maximum-am-resource-percent': '0.25',
    },
}, merge=salt['pillar.get']('data:properties:capacity-scheduler'))}) %}
