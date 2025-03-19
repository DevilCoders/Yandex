extend:
    adapter-container:
        docker_container.running:
            - require_in:
                  - pkg: dbaas-worker-pkgs
                  - pkg: mdb-internal-api-pkgs
                  - pkg: dbaas-internal-api-pkgs
                  - pkg: supervisor-packages
                  - file: populate-table

    gateway-container:
        docker_container.running:
            - require_in:
                  - pkg: dbaas-worker-pkgs
                  - pkg: mdb-internal-api-pkgs
                  - pkg: dbaas-internal-api-pkgs
                  - pkg: supervisor-packages
                  - file: populate-table

    configserver-container:
        docker_container.running:
            - require_in:
                  - pkg: dbaas-worker-pkgs
                  - pkg: mdb-internal-api-pkgs
                  - pkg: dbaas-internal-api-pkgs
                  - pkg: supervisor-packages
                  - file: populate-table

order-components:
    test.nop:
        - require:
            - sls: components.pg-dbs.dbaas_metadb
        - require_in:
            - sls: components.dbaas-worker

