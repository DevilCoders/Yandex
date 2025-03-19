data:
    monrun2: True
    diagnostic_tools: False
    l3host: True
    ipv6selfdns: True
    runlist:
        - components.walg-server
    sysctl:
        vm.nr_hugepages: 0

include:
    - envs.prod
    - porto.prod.hw_watcher
    - porto.prod.walg
    - porto.prod.selfdns.realm-mdb
    - mdb_backup.{{ salt['grains.get']('id').replace('.', '_') }}
