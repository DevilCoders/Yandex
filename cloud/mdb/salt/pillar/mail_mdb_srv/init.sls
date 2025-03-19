mine_functions:
    grains.item:
        - id
        - role
        - ya

data:
    l3host: True
    network_autoconf: True
    runlist:
        - components.dom0porto
        - components.lazy-trim
    array_for_sata: /dev/md/3
    monrun2: True
    dist:
        bionic:
            secure: False
    mdb_metrics:
        main:
            yasm_tags_cmd: /usr/local/yasmagent/dom0porto_getter.py
    lazy-trim:
        enabled: True
        bandwidth: 257 # Maximum TRIM speed 257 MB/s
    config:
        dbm_project_id: '0:640'
        fill_page_cache: True
    linux_kernel:
        version: '4.4.171-70.8'
    hw_watcher:
        repair_bad_sectors: 'no'

include:
    - envs.prod
    - porto.prod.selfdns.realm-mail
    - porto.prod.dom0.dom0porto
    - porto.prod.hw_watcher
    - mdb_metrics.solomon_intra_dom0
