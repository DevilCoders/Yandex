mine_functions:
    grains.item:
        - id
        - role
        - ya

data:
    l3host: True
    use_vlan688: True
    use_atop: False
    runlist:
        - components.dom0porto
        - components.lazy-trim
        - components.netmon
        - components.gideon
    dist:
        bionic:
            secure: False
    dom0porto:
        use_dbaas_dom0_images: True
        image_bucket: 'dbaas-images-vm-built'
    sysctl:
        vm.max_map_count: 2621440
        kernel.core_pattern: '|/usr/sbin/portod core %P %I %p %i %s %d %c /var/cores/%e.%p.%s %u %g'
    network_autoconf: True
    monrun2: True
    monrun:
        unispace:
            exact_paths: '/ /data /slow'
        load_average:
            warn: 500
            crit: 1000
    array_for_sata: /dev/md/3
    lazy-trim:
        enabled: True
        bandwidth: 384 # Maximum TRIM speed
    mdb_metrics:
        sys_common: False
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py # MDB metrics unable to work correct with dom0porto_getter.py
    salt_minion:
        configure_s3: True
    linux_kernel:
        version: '4.19.183-42.1mofed'
    config:
        old_backups_threashold: 604800
    wall-e:
        repair: True
    hw_watcher:
        repair_bad_sectors: 'no'

include:
    - envs.prod
    - porto.prod.selfdns.realm-mdb
    - porto.prod.s3.dbaas_dom0_images
    - porto.prod.dom0.dom0porto
    - porto.prod.hw_watcher
    - mdb_metrics.solomon_intra_dom0
    - porto.prod.images
    - porto.prod.gideon
    - netmon
