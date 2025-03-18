import pytest

import tools.cfggen.generators.basesearch as basesearch


def test_configs_count(curdb):
    group = curdb.groups.get_group('VLA_VIDEO_PLATINUM_BASE')
    t = basesearch.TBasesearchGenerator().build(group)

    configs_per_instance = len(
        set(['template', 'protoconfig']) & set(dir(group.card.configs.basesearch))
    )

    unique_configs_names = set()
    for entry in t.report_entries:
        unique_configs_names.update(entry.fnames)

    assert len(unique_configs_names) == configs_per_instance * len(group.get_kinda_busy_instances())


def test_instance_replicas(curdb):
    group = curdb.groups.get_group('VLA_VIDEO_PLATINUM_BASE')
    generator = basesearch.TBasesearchGenerator()
    generator.build(group)

    assert len(generator.template_data['instance_replicas']) == len(group.get_kinda_busy_instances())


def test_replicas_not_empty(curdb):
    group = curdb.groups.get_group('VLA_VIDEO_PLATINUM_BASE')
    generator = basesearch.TBasesearchGenerator()
    generator.build(group)

    for replicas in generator.template_data['instance_replicas'].values():
        assert len(replicas) > 1


def test_host_cores(curdb):
    group = curdb.groups.get_group('SAS_IMGS_BASE')
    generator = basesearch.TBasesearchGenerator()
    generator.build(group)

    for host in group.get_kinda_busy_hosts():
        assert generator.template_data['host_cores'][host] >= 0
