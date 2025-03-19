import pytest
import ydputils as ydp


cores_per_map_task = 1
cores_per_reduce_task = 2
cores_per_app_master = 2
spark_driver_memory_fraction = 0.25
spark_driver_result_size_ratio = 0.5
nodemanager_available_memory_ratio = 0.8

GiB = 1024

UNIFORM = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
    'compute-subcluster-id': {
        'role': ydp.ROLE_COMPUTENODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
}

LARGE_MASTER = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 32,
            'memory': 137438953472,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
}

SMALL_MASTER = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
}

VARIOUS_DATA_AND_COMPUTE = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
    'compute-subcluster-id': {
        'role': ydp.ROLE_COMPUTENODE,
        'resources': {
            'cores': 16,
            'memory': 68719476736,
        },
    },
}

SMALL_COMPUTE = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
    'compute-subcluster-id': {
        'role': ydp.ROLE_COMPUTENODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
    },
}

WITHOUT_NODEMANAGERS = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
    }
}

WITHOUT_CORES = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
        'subcid': 'master-subcluster-id',
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'memory': 4294967296,
        },
        'subcid': 'data-subcluster-id',
    },
}

WITHOUT_MEMORY = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 1,
            'memory': 4294967296,
        },
        'subcid': 'master-subcluster-id',
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_DATANODE,
        'resources': {
            'cores': 1,
        },
        'subcid': 'data-subcluster-id',
    },
}

LIGHTWEIGHT_SPARK = {
    'master-subcluster-id': {
        'role': ydp.ROLE_MASTERNODE,
        'resources': {
            'cores': 12,
            'memory': 51539607552,
        },
    },
    'data-subcluster-id': {
        'role': ydp.ROLE_COMPUTENODE,
        'resources': {
            'cores': 8,
            'memory': 34359738368,
        },
    },
}


def test_scheduler_settings_for_uniform_cluster():
    subclusters = UNIFORM
    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 25 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 8
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 25 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 8

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 2
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 6 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 2
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 6 * GiB
    )

    assert ydp.get_spark_deploy_mode(subclusters) == 'cluster'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 2
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_memory_mb(
        subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio
    ) == 12 * GiB - int(12 * GiB / 10)
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 25 * GiB * spark_driver_memory_fraction
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 3200
    )


def test_scheduler_settings_for_large_master_cluster():
    subclusters = LARGE_MASTER
    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 25 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 8
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 25 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 8

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 2
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 6 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 2
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 6 * GiB
    )

    assert ydp.get_spark_deploy_mode(subclusters) == 'cluster'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 2
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_memory_mb(
        subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio
    ) == 12 * GiB - int(12 * GiB / 10)
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 25 * GiB * spark_driver_memory_fraction
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 3200
    )


def test_scheduler_settings_for_small_master_cluster():
    subclusters = SMALL_MASTER
    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 25 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 8
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 25 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 8

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 2
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 6 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 2
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 6 * GiB
    )

    assert ydp.get_spark_deploy_mode(subclusters) == 'cluster'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 2
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_memory_mb(
        subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio
    ) == 12 * GiB - int(12 * GiB / 10)
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 25 * GiB * spark_driver_memory_fraction
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 3200
    )


def test_scheduler_settings_for_various_data_and_compute_cluster():
    subclusters = VARIOUS_DATA_AND_COMPUTE
    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 51 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 16
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 25 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 8

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 2
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 6 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 2
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 6 * GiB
    )

    assert ydp.get_spark_deploy_mode(subclusters) == 'cluster'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 2
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 8 / spark_executors_per_vm
    assert ydp.get_spark_executor_memory_mb(
        subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio
    ) == 12 * GiB - int(12 * GiB / 10)
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 25 * GiB * spark_driver_memory_fraction
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 3200
    )


def test_scheduler_settings_for_small_compute_cluster():
    subclusters = SMALL_COMPUTE
    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 25 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 8
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 3 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 1

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 1
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 3 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 1
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 3 * GiB
    )

    assert ydp.get_spark_deploy_mode(subclusters) == 'cluster'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 2
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 1
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 1
    assert (
        ydp.get_spark_executor_memory_mb(subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio)
        == 1 * GiB - 384
    )
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 768
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 384
    )


def test_scheduler_settings_for_lightweight_cluster():
    subclusters = LIGHTWEIGHT_SPARK

    assert ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 1 * GiB
    assert ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio) == 25 * GiB
    assert ydp.get_yarn_sched_max_allocation_vcores(subclusters) == 8
    assert (
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
        == 25 * GiB
    )
    assert ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) == 8

    assert ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task) == 1
    assert (
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio) == 3 * GiB
    )
    assert ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task) == 2
    assert (
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
        == 6 * GiB
    )
    assert ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master) == 2
    assert (
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
        == 6 * GiB
    )

    assert ydp.get_hive_warehouse_path(['yarn', 'spark'], 'my-bucket') == 's3a://my-bucket/warehouse'

    assert ydp.get_spark_deploy_mode(subclusters) == 'client'
    spark_executors_per_vm = ydp.get_default_spark_executors_per_vm(subclusters)
    assert spark_executors_per_vm == 1
    assert ydp.get_spark_executor_cores(subclusters, spark_executors_per_vm) == 8
    # get_yarn_sched_max_allocation_mb_for_all_instances - 10% for spark off-heap overhead
    assert ydp.get_spark_executor_memory_mb(
        subclusters, spark_executors_per_vm, nodemanager_available_memory_ratio
    ) == 25 * GiB - (25 * GiB / 10)
    # 38 GiB is 80% of total 48GiB memory
    # 4 GiB is 10% off-heap overhead for driver
    assert (
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
        == 38 * GiB - 4 * GiB
    )
    assert (
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
        == 4096
    )
    assert ydp.get_spark_driver_cores(subclusters, spark_executors_per_vm) == 10


def test_scheduler_settings_for_cluster_without_cores():
    subclusters = WITHOUT_CORES
    default_spark_executors = ydp.get_default_spark_executors_per_vm(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_vcores(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)

    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master)

    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_driver_cores(subclusters, default_spark_executors)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_executor_cores(subclusters, default_spark_executors)


def test_scheduler_settings_for_cluster_without_memory():
    subclusters = WITHOUT_MEMORY
    default_spark_executors = ydp.get_default_spark_executors_per_vm(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_executor_memory_mb(subclusters, default_spark_executors, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )


def test_scheduler_settings_for_cluster_without_nodemanagers():
    subclusters = WITHOUT_NODEMANAGERS
    default_spark_executors = ydp.get_default_spark_executors_per_vm(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_vcores(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)

    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_map_memory_mb(subclusters, cores_per_map_task, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_reduce_memory_mb(subclusters, cores_per_reduce_task, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_appmaster_memory_mb(subclusters, cores_per_app_master, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_map_cores(subclusters, cores_per_map_task)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_reduce_cores(subclusters, cores_per_reduce_task)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_mapreduce_appmaster_cores(subclusters, cores_per_app_master)

    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_executor_cores(subclusters, default_spark_executors)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_executor_memory_mb(subclusters, default_spark_executors, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
    with pytest.raises(ydp.DataprocWrongPillarException) as e:  # noqa
        ydp.get_spark_driver_result_size_mem_mb(
            subclusters,
            spark_driver_memory_fraction,
            spark_driver_result_size_ratio,
            nodemanager_available_memory_ratio,
        )
