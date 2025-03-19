data:
    mongodb:
        users:
            admin:
                password: {{ salt.yav.get('ver-01eaa2fc44a1tjtqhafxraa8qp[password]') }}
                services:
                    - mongod
                    - mongos
                    - mongocfg
                dbs:
                    admin:
                        - "root"
            monitor:
                password: {{ salt.yav.get('ver-01eaa2h39gfr7gev78jd30c5a6[password]') }}
                services:
                    - mongod
                    - mongos
                    - mongocfg
                dbs:
                    admin:
                        - "clusterMonitor"
            test_user:
                password: {{ salt.yav.get('ver-01eaa2jhjzpc3z09sxpsfbw0tq[password]') }}
                services:
                    - mongod
                    - mongos
                    - mongocfg
                dbs:
                    testdb1:
                        - "readWrite"
        keyfile: |
            {{ salt.yav.get('ver-01eaa2nk2y2hfqfqh0tev0whrw[private]') | indent(12) }}
