OWNER(g:cloud-infra)

PY3TEST()

PY_SRCS(
    coredump_common.py
    cpu_topology.py
    dig.py
    dns_check.py
    fragile_disks.py
    freespace.py
    hw_watcher.py
    kikimr.py
    kikimr_disks.py
    load_average.py
    mounts.py
    nic_health.py
    ntp.py
    oom-killer.py
    ownerless_packages.py
    packages_version.py
    pstore_is_empty.py
    raid.py
    reboot-count.py
    repo.py
    solomon-agent.py
    tpm.py
    unbound.py
    vf_count.py
    walle_fs_check.py
)

NO_CHECK_IMPORTS()

END()
