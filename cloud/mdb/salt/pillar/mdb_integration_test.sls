mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

include:
    - envs.dev
    - porto.prod.selfdns.realm-sandbox
    - porto.prod.s3.dbaas_dom0_images
    - porto.prod.s3.ext_images_upload
    - porto.prod.dbaas.kvm_template
    - compute.prod.image_upload
    - porto.prod.hw_watcher
    - porto.prod.arcadia-build-node
    - porto.prod.buildvm
    - porto.prod.docker
    - porto.prod.gideon

data:
    l3host: True
    monrun2: True
    cauth_use_v2: True
    use_unbound_64: True
    docker_users:
        - secwall
        - d0uble
        - arhipov
        - dsarafan
        - efimkin
        - 'alex-burmak'
        - wizard
        - schizophrenia
        - x4mmm
        - reshke
        - denchick
        - ein-krebs
        - mialinx
        - annkpx
        - sidh
        - pperekalov
        - vgoshev
        - vlbel
        - peevo
        - pervakovg
        - frisbeeman
        - ayasensky
        - irioth
        - velom
        - estrizhnev
        - waleriya
        - dstaroff
        - vadim-volodin
        - rivkind-nv
        - vorobyov-as
        - stewie
        - andrei-vlg
        - fedusia
        - sageneralova
        - robert
        - prez
        - teem0n
        - dkhurtin
        - vicpopov
        - iantonspb
        - egor-medvedev
        - moukavoztchik
        - roman-chulkov
        - mariadyuldina
        - xifos
        - yoschi
    buildvm:
        dist_map:
            bionic: mdb-bionic
    linux_kernel:
        version: '5.4.196-37.1'
    lazy-trim:
        enabled: True
        bandwidth: 128
        path: /,/u0
    runlist:
        - components.common
        - components.buildvm
        - components.docker-host
        - components.kvm-host
        - components.linux-kernel
        - components.lazy-trim
        - components.repositories.apt.search-kernel
        - components.gideon
        - components.graphene-load-node
