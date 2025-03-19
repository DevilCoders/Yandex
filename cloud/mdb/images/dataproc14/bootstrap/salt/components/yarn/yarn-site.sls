{% set config_site = {} %}

{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set masternode = salt['ydputils.get_masternodes']()[0] %}
{% set round = salt['ydputils.round_down'] %}

{% set total_cores = salt['grains.get']('num_cpus') %}
{% set total_memory = salt['grains.get']('mem_total') %}

{% set yarn_cores = [1, (total_cores | int)] | max %}

{% set memory_mb = round(total_memory, 1024) %}
{% set available_memory_ratio = 0.8 %}
{% set available_memory_mb = (total_memory * available_memory_ratio) | int %}
{% set yarn_nodemanager_min_allocation = round(available_memory_mb, 256, maximum = 1024) %}
{% set yarn_nodemanager_max_allocation = round(available_memory_mb, yarn_nodemanager_min_allocation) %}

{% do config_site.update({'yarn': salt['grains.filter_by']({
    'Debian': {
        'yarn.acl.enable': 'false',
        'yarn.admin.acl': '*',
        'yarn.log-aggregation-enable': 'true',
        'yarn.log-aggregation.retain-seconds': 604800,

        'yarn.resourcemanager.hostname': masternode,
        'yarn.resourcemanager.fs.state-store.uri': 'file:///hadoop/yarn/system/rmstore',
        'yarn.resourcemanager.nodes.exclude-path': hadoop_config_path + '/yarn-nodes.exclude',
        'yarn.nodemanager.log-dirs': '/var/log/hadoop-yarn/containers',
        'yarn.nodemanager.remote-app-log-dir': '/var/log/hadoop-yarn/apps',
        'yarn.nodemanager.local-dirs': '/hadoop/yarn/nm-local-dir',
        'yarn.nodemanager.resource.memory-mb': yarn_nodemanager_max_allocation,
        'yarn.nodemanager.vmem-check-enabled': 'false',
        'yarn.nodemanager.resource.cpu-vcores': yarn_cores,
        'yarn.nodemanager.aux-services': {
            'append': 'true',
            'value': 'mapreduce_shuffle'
        },
        'yarn.nodemanager.aux-services.mapreduce_shuffle.class': 'org.apache.hadoop.mapred.ShuffleHandler',
        'yarn.nodemanager.delete.debug-delay-sec': 300,

        'yarn.scheduler.minimum-allocation-mb': salt['ydputils.get_yarn_sched_min_allocation_mb'](),
        'yarn.scheduler.maximum-allocation-mb': salt['ydputils.get_yarn_sched_max_allocation_mb'](),
        'yarn.scheduler.maximum-allocation-vcores': salt['ydputils.get_yarn_sched_max_allocation_vcores'](),

        'yarn.timeline-service.http-cross-origin.enabled': 'true',
        'yarn.timeline-service.enabled': 'true',
        'yarn.timeline-service.hostname': masternode,
        'yarn.timeline-service.bind-host': masternode,
        'yarn.timeline-service.generic-application-history.enabled': 'true',
        'yarn.timeline-service.recovery.enabled': 'true'
    },
}, merge=salt['pillar.get']('data:properties:yarn'))}) %}
