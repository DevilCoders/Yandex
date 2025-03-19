pmm:
  pmm_server: 'pmm.server.fqdn'
  pmm_server_port: 80
  # DB type is 'mongo' or 'mysql'
  pmm_db_type: mongo
  # if MongoDB then provide next settings
  pmm_mongo_cluster: 'mongo_cluster_name'
  pmm_mongo_port: 27018
  # MongoDB user 'monitoring' password is getting from "/var/cache/mongo-grants/mongo_grants_config"
  # See https://www.percona.com/doc/percona-monitoring-and-management/conf-mongodb.html
  #
  # Bring custom MySQL config
  # See https://www.percona.com/doc/percona-monitoring-and-management/conf-mysql.html
  use_best_results_mysql_config: true
