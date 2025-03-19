root:
    repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
    repo_path: /cloud/mdb/salt
    mount_path: /srv

fileroots:
    repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
    repo_path: /cloud/mdb/salt/salt

fileroots_envs_mount_path_dev_env: /srv/salt
fileroots_envs_mount_path: /srv/envs

fileroots_subrepos:
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /security/apparmor
      mount_path: components/apparmor-profiles
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/pg
      mount_path: components/pg-code
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/dbaas_metadb
      mount_path: components/pg-code/dbaas_metadb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/deploydb
      mount_path: components/pg-code/deploydb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/secretsdb
      mount_path: components/pg-code/secretsdb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /mail/pg/sharddb
      mount_path: components/pg-code/sharddb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/dbm/dbmdb
      mount_path: components/pg-code/dbm
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/katan/db
      mount_path: components/pg-code/katandb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/cms/db
      mount_path: components/pg-code/cmsdb
    - repo_uri: svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia
      repo_path: /cloud/mdb/mlockdb
      mount_path: components/pg-code/mlockdb
