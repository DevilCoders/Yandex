mine_functions:
    grains.item:
        - id
        - role
        - ya

data:
    deploy:
        version: 2
        api_host: deploy-api-test.db.yandex-team.ru
    dist:
        bionic:
            secure: False
    l3host: True
    use_vlan688: True
    use_atop: False
    runlist:
        - components.dom0porto
        - components.lazy-trim
        - components.netmon
        - components.gideon
    dom0porto:
        use_dbaas_dom0_images: True
        image_bucket: 'dbaas-images-vm-built-test'
    porto_version: '5.2.8'
    sysctl:
      vm.max_map_count: 2621440
      kernel.core_pattern: '|/usr/sbin/portod core %P %I %p %i %s %d %c /var/cores/%e.%p.%s %u %g'
    network_autoconf: True
    monrun2: True
    monrun:
        unispace:
            exact_paths: '/ /data /slow'
    mdb_metrics:
        sys_common: False
        main:
            yasm_tags_cmd: /usr/local/yasmagent/default_getter.py # MDB metrics unable to work correct with dom0porto_getter.py
    salt_minion:
        configure_s3: True
    cauth_use_v2: True
    config:
        old_backups_threashold: 7200
        dbm_heartbeat_override:
            project: pgaas
            generation: {{ 1 if '0001' in salt['grains.get']('id') else 3 }}
    linux_kernel:
        version: '4.19.183-42.1mofed'

include:
    - envs.dev
    - porto.test.selfdns.realm-mdb
    - porto.test.s3.dbaas_dom0_images
    - porto.test.dom0
    - porto.test.hw_watcher
    - mdb_metrics.solomon_intra_dom0
    - porto.test.images
    - porto.prod.gideon
    - netmon
