"clusters":
  "pgmeta01":
    "fqdns":
    - "pgmeta01-01-il1-a.mdb-s3.yandexcloud.co.il"
    - "pgmeta01-02-il1-a.mdb-s3.yandexcloud.co.il"
    "resources":
      "core_fraction": 50
      "cores": 2
      "memory": 8
    "service_account_id": "yc.database.pgmeta01-cluster"
  "s3db01":
    "fqdns":
    - "s3db01-01-il1-a.mdb-s3.yandexcloud.co.il"
    - "s3db01-02-il1-a.mdb-s3.yandexcloud.co.il"
    - "s3db01-03-il1-a.mdb-s3.yandexcloud.co.il"
    "resources":
      "core_fraction": 100
      "cores": 4
      "memory": 16
    "service_account_id": "yc.database.s3db01-cluster"
  "s3meta01":
    "fqdns":
    - "s3meta01-01-il1-a.mdb-s3.yandexcloud.co.il"
    - "s3meta01-02-il1-a.mdb-s3.yandexcloud.co.il"
    - "s3meta01-03-il1-a.mdb-s3.yandexcloud.co.il"
    "resources":
      "core_fraction": 100
      "cores": 4
      "memory": 16
    "service_account_id": "yc.database.s3meta01-cluster"
  "zk01":
    "fqdns":
    - "zk01-01-il1-a.mdb-s3.yandexcloud.co.il"
    - "zk01-02-il1-a.mdb-s3.yandexcloud.co.il"
    - "zk01-03-il1-a.mdb-s3.yandexcloud.co.il"
    "resources":
      "core_fraction": 50
      "cores": 2
      "memory": 8
    "service_account_id": "yc.database.zk01-cluster"
