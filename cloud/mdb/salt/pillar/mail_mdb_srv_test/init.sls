mine_functions:
    grains.item:
        - id
        - role
        - ya

data:
    monrun2: True
    l3host: True
    network_autoconf: True
    array_for_sata: /dev/md/3
    use_atop: False
    lazy-trim:
        enabled: True
        bandwidth: 256 # Maximum TRIM speed 256 MB/s
    dist:
        bionic:
            secure: False
    dom0porto:
        use_dbaas_dom0_images: True
    runlist:
        - components.dom0porto
        - components.lazy-trim
    mdb_metrics:
        sys_common: False
        main:
            yasm_tags_cmd: /usr/local/yasmagent/dom0porto_getter.py
    config:
        dbm_project_id: '0:639'
    linux_kernel:
        version: '4.19.143-37'

include:
    - envs.dev
    - porto.prod.selfdns.realm-sandbox
    - porto.prod.s3.dbaas_dom0_images
    - porto.prod.dom0.dom0porto
    - porto.prod.hw_watcher
    - mdb_metrics.solomon_intra_dom0
