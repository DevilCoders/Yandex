data:
    monrun2: True
    diagnostic_tools: False
    l3host: True
    ipv6selfdns: True
    runlist:
        - components.walg-server
    sysctl:
        vm.nr_hugepages: 0
    walg-server:
        clusters:
            - s3meta-test01
            - s3meta-test02
            - s3db-test01
            - s3db-test02

include:
    - envs.dev
    - porto.test.walg
    - porto.test.selfdns.realm-mdb
