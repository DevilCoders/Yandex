mine_functions:
    grains.item:
      - id
      - role
      - ya

data:
    runlist:
        - components.mdb-tank
    salt_version: 3001.7+ds-1+yandex0
    salt_py_version: 3

include:
    - envs.prod
    - compute.prod.selfdns.realm-cloud-mdb
