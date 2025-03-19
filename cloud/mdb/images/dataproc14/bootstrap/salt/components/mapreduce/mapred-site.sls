{% set config_site = {} %}

{% set nodes = salt['ydputils.get_nodemanagers_count']() %}
{% set masternode = salt['ydputils.get_masternodes']()[0] %}

# Load settings
{% set cores_per_map_task = salt['ydputils.get_mapreduce_map_cores']() %}
{% set cores_per_reduce_task = salt['ydputils.get_mapreduce_reduce_cores']() %}
{% set cores_per_app_master = salt['ydputils.get_mapreduce_appmaster_cores']() %}
{% set heap_ratio = 0.8 %}

{% set default_maps = nodes * 10 %}
{% set default_reduces = nodes * 4 %}
{% set max_cores = salt['ydputils.get_yarn_sched_max_allocation_vcores_for_all_instances'] () %}
{% set map_slots = [1, ((max_cores / cores_per_map_task) | int)] | max %}

{% set map_mem_mb = salt['ydputils.get_mapreduce_map_memory_mb']() %}
{% set reduce_mem_mb = salt['ydputils.get_mapreduce_reduce_memory_mb']() %}
{% set app_master_mem_mb = salt['ydputils.get_mapreduce_appmaster_memory_mb']() %}

{% set map_mem_heap = ((map_mem_mb * heap_ratio) | int) | string %}
{% set reduce_mem_heap = ((reduce_mem_mb * heap_ratio) | int) | string %}
{% set app_master_mem_heap = ((app_master_mem_mb * heap_ratio) | int) | string %}

{% do config_site.update({'mapred': salt['grains.filter_by']({
    'Debian': {
        'mapreduce.framework.name': 'yarn',
        'mapred.compress.map.output': 'true',
        'mapred.child.env': 'JAVA_LIBRARY_PATH=$JAVA_LIBRARY_PATH:/usr/lib/hadoop/lib/native',
        'mapreduce.map.memory.mb': map_mem_mb,
        'mapreduce.reduce.memory.mb': reduce_mem_mb,
        'yarn.app.mapreduce.am.resource.mb': app_master_mem_mb,
        'mapreduce.map.java.opts': '-Xmx' + map_mem_heap + 'm',
        'mapreduce.reduce.java.opts': '-Xmx' + reduce_mem_heap + 'm',
        'yarn.app.mapreduce.am.command-opts': '-Xmx' + app_master_mem_heap + 'm',
        'mapreduce.map.cpu.vcores': cores_per_map_task,
        'mapreduce.reduce.cpu.vcores': cores_per_reduce_task,
        'yarn.app.mapreduce.am.resource.cpu-vcores': cores_per_app_master,
        'mapreduce.job.maps': default_maps,
        'mapreduce.job.reduces': default_reduces,
        'mapreduce.tasktracker.map.tasks.maximum': map_slots,
        'mapreduce.jobhistory.recovery.enable': 'true',
        'mapreduce.jobhistory.webapp.address': masternode + ':19888',
        'mapreduce.jobhistory.webapp.https.address': masternode + ':19890',
        'mapreduce.jobhistory.address': masternode + ':10020',
        'mapreduce.fileoutputcommitter.algorithm.version': '2',
        'mapreduce.fileoutputcommitter.task.cleanup.enabled': 'true'
    },
}, merge=salt['pillar.get']('data:properties:mapreduce'))}) %}
