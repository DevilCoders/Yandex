import copy

import importlib

agent = importlib.import_module("dataproc-agent")


def test_get_max_concurrent_jobs():
    default_reserved_memory_for_services = 5 * 1024 * 1024 * 1024
    default_job_memory_footprint = 512 * 1024 * 1024

    masternode_8gib_pillar = {
        'topology': {
            'subclusters': {
                'main_subcluster_id': {
                    'resources': {
                        'memory': 8 * 1024 * 1024 * 1024,
                    },
                },
            },
        },
        'subcluster_main_id': 'main_subcluster_id',
    }

    assert (
        agent.get_max_concurrent_jobs(
            default_reserved_memory_for_services,
            default_job_memory_footprint,
            pillar=masternode_8gib_pillar,
        )
        == 6
    )

    masternode_16gib_pillar = copy.deepcopy(masternode_8gib_pillar)
    ram_16_gib = 16 * 1024 * 1024 * 1024
    masternode_16gib_pillar['topology']['subclusters']['main_subcluster_id']['resources']['memory'] = ram_16_gib
    assert (
        agent.get_max_concurrent_jobs(
            default_reserved_memory_for_services,
            default_job_memory_footprint,
            pillar=masternode_16gib_pillar,
        )
        == 22
    )

    masternode_8gib_pillar_with_property = copy.deepcopy(masternode_8gib_pillar)
    masternode_8gib_pillar_with_property['properties'] = {'dataproc': {'max-concurrent-jobs': 666}}
    assert (
        agent.get_max_concurrent_jobs(
            default_reserved_memory_for_services,
            default_job_memory_footprint,
            pillar=masternode_8gib_pillar_with_property,
        )
        == 666
    )

    reserved_memory_for_services = 4 * 1024 * 1024 * 1024
    job_memory_footprint = 256 * 1024 * 1024
    masternode_8gib_pillar_with_properties = copy.deepcopy(masternode_8gib_pillar)
    masternode_8gib_pillar_with_properties['properties'] = {
        'dataproc': {
            'job-memory-footprint': reserved_memory_for_services,
            'reserved-memory-for-services': job_memory_footprint,
        }
    }
    assert (
        agent.get_max_concurrent_jobs(
            reserved_memory_for_services,
            job_memory_footprint,
            pillar=masternode_8gib_pillar_with_properties,
        )
        == 16
    )

    masternode_4gib_pillar = copy.deepcopy(masternode_8gib_pillar)
    ram_4_gib = 4 * 1024 * 1024 * 1024
    masternode_4gib_pillar['topology']['subclusters']['main_subcluster_id']['resources']['memory'] = ram_4_gib
    assert (
        agent.get_max_concurrent_jobs(
            default_reserved_memory_for_services,
            default_job_memory_footprint,
            pillar=masternode_4gib_pillar,
        )
        == 1
    )
