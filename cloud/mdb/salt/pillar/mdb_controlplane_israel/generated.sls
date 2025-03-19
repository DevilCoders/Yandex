"clusters":
  "deploy_salt":
    "backups_bucket": ""
    "fqdns":
    - "deploy-salt01-il1-a.mdb-cp.yandexcloud.co.il"
    - "deploy-salt02-il1-a.mdb-cp.yandexcloud.co.il"
    "resources":
      "core_fraction": 100
      "cores": 16
      "memory": 16
    "service_account_id": "yc.mdb.deploy-salt-cluster"
    "users": {}
  "deploydb":
    "backups_bucket": "yc-mdb-deploydb-backups"
    "fqdns":
    - "deploydb01-il1-a.mdb-cp.yandexcloud.co.il"
    - "deploydb02-il1-a.mdb-cp.yandexcloud.co.il"
    - "deploydb03-il1-a.mdb-cp.yandexcloud.co.il"
    "resources":
      "core_fraction": 100
      "cores": 2
      "memory": 8
    "service_account_id": "yc.mdb.deploydb-cluster"
    "users":
      "admin":
        "secret_id": "bcnevrvq9rg9brcmbr87"
        "secret_key": "password"
      "deploy_api":
        "secret_id": "bcn9j6abdairqop8j34a"
        "secret_key": "password"
      "deploy_cleaner":
        "secret_id": "bcng6a19i2j6lpimbjmq"
        "secret_key": "password"
      "deploydb_admin":
        "secret_id": "bcnv2c260ma2fiqesp9e"
        "secret_key": "password"
      "mdb_ui":
        "secret_id": "bcn29os8smmb8s2btetf"
        "secret_key": "password"
      "monitor":
        "secret_id": "bcnngt9bbr9jn9q71mbg"
        "secret_key": "password"
      "postgres":
        "secret_id": "bcnpumq69livbu8p10bm"
        "secret_key": "password"
      "repl":
        "secret_id": "bcndmldhj2k99tg59kjg"
        "secret_key": "password"
  "metadb":
    "backups_bucket": "yc-mdb-metadb-backups"
    "fqdns":
    - "metadb01-il1-a.mdb-cp.yandexcloud.co.il"
    - "metadb02-il1-a.mdb-cp.yandexcloud.co.il"
    - "metadb03-il1-a.mdb-cp.yandexcloud.co.il"
    "resources":
      "core_fraction": 100
      "cores": 2
      "memory": 8
    "service_account_id": "yc.mdb.metadb-cluster"
    "users":
      "admin":
        "secret_id": "bcn3dg5eh1g5onpj7cvp"
        "secret_key": "password"
      "backup_cli":
        "secret_id": "bcngc8pqifdp0c0u8h8j"
        "secret_key": "password"
      "backup_scheduler":
        "secret_id": "bcn8vofgab48vhjqjq6s"
        "secret_key": "password"
      "backup_worker":
        "secret_id": "bcneoakvusv2gq6hjsnd"
        "secret_key": "password"
      "billing_bookkeeper":
        "secret_id": "bcngc9q89kfbt728dq24"
        "secret_key": "password"
      "cloud_dwh":
        "secret_id": "bcnegotkh7beiknuaf95"
        "secret_key": "password"
      "cms":
        "secret_id": "bcnu2qjtjcldhrovj7ng"
        "secret_key": "password"
      "dataproc_health":
        "secret_id": "bcnd0p0fr47lbmu40nln"
        "secret_key": "password"
      "dbaas_api":
        "secret_id": "bcn7cn358s89a3uhpd86"
        "secret_key": "password"
      "dbaas_support":
        "secret_id": "bcnp9fqbfm1635ni3su1"
        "secret_key": "password"
      "dbaas_worker":
        "secret_id": "bcnl7f0t93e9q0oe5cqr"
        "secret_key": "password"
      "idm_service":
        "secret_id": "bcnti130coqaageh5kgh"
        "secret_key": "password"
      "katan_imp":
        "secret_id": "bcnuo2l0hr753896324p"
        "secret_key": "password"
      "logs_api":
        "secret_id": "bcnqjmpb9fe5l6toj671"
        "secret_key": "password"
      "mdb_dns":
        "secret_id": "bcnjoiuf0bl17gtdhqaj"
        "secret_key": "password"
      "mdb_downtimer":
        "secret_id": "bcnu42qab5eb4ufljp8g"
        "secret_key": "password"
      "mdb_event_producer":
        "secret_id": "bcniccir5uj93t5sl4ni"
        "secret_key": "password"
      "mdb_exporter":
        "secret_id": "bcn2u0npvqbuvqhen4s9"
        "secret_key": "password"
      "mdb_health":
        "secret_id": "bcn1n6kgb38o9f33oc6h"
        "secret_key": "password"
      "mdb_maintenance":
        "secret_id": "bcn425t8lm7trfjbvcjr"
        "secret_key": "password"
      "mdb_reaper":
        "secret_id": "bcnf0flvnvt2hlmsuuuo"
        "secret_key": "password"
      "mdb_report":
        "secret_id": "bcnafmpkjj4qp1p93cfm"
        "secret_key": "password"
      "mdb_search_producer":
        "secret_id": "bcn83n0d34a0khj19h8h"
        "secret_key": "password"
      "mdb_ui":
        "secret_id": "bcnfj6gp0238pa9dsnnd"
        "secret_key": "password"
      "metadb_admin":
        "secret_id": "bcn5v8147ol9487en91r"
        "secret_key": "password"
      "monitor":
        "secret_id": "bcnsg0ggdj3rrnn0g0do"
        "secret_key": "password"
      "pillar_config":
        "secret_id": "bcnc54641rd8g9m4t3vi"
        "secret_key": "password"
      "pillar_secrets":
        "secret_id": "bcnri7ujl6rcllsdt44m"
        "secret_key": "password"
      "postgres":
        "secret_id": "bcn5mpucli9nrlpbifej"
        "secret_key": "password"
      "repl":
        "secret_id": "bcnoee8sblvrlm17eq55"
        "secret_key": "password"
  "secretsdb":
    "backups_bucket": "yc-mdb-secretsdb-backups"
    "fqdns":
    - "secretsdb01-il1-a.mdb-cp.yandexcloud.co.il"
    - "secretsdb02-il1-a.mdb-cp.yandexcloud.co.il"
    - "secretsdb03-il1-a.mdb-cp.yandexcloud.co.il"
    "resources":
      "core_fraction": 50
      "cores": 2
      "memory": 8
    "service_account_id": "yc.mdb.secretsdb-cluster"
    "users":
      "admin":
        "secret_id": "bcn95h9esmb5oionpoa6"
        "secret_key": "password"
      "monitor":
        "secret_id": "bcn1bt2m457d7iph5r5a"
        "secret_key": "password"
      "postgres":
        "secret_id": "bcnfqa3jj6bmtr6utt15"
        "secret_key": "password"
      "repl":
        "secret_id": "bcnoofpu0lnm4e6g7uec"
        "secret_key": "password"
      "secrets_api":
        "secret_id": "bcndmtc8esnfjr4olb2h"
        "secret_key": "password"
      "secretsdb_admin":
        "secret_id": "bcnhecj1ptqv9396or44"
        "secret_key": "password"
  "zk01":
    "backups_bucket": ""
    "fqdns":
    - "zk01-01-il1-a.mdb-cp.yandexcloud.co.il"
    - "zk01-02-il1-a.mdb-cp.yandexcloud.co.il"
    - "zk01-03-il1-a.mdb-cp.yandexcloud.co.il"
    "resources":
      "core_fraction": 50
      "cores": 2
      "memory": 8
    "service_account_id": "yc.mdb.zk01-cluster"
    "users": {}
"salt_images_bucket": "yc-mdb-salt-images"
