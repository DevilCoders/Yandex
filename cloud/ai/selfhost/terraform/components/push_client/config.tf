locals {
  master_address_by_environment = {
    # TODO: Add correct master adress after creation  ofdb in the cloud preprod
    preprod = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net"
    staging = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net"
    prod    = "lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net"
  }

  database_by_environment = {
    # TODO: Add correct database after creation of db in the cloud preprod
    preprod = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"
    staging = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"
    prod    = "/global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla"
  }

  config = {
    network = {
      master-addr = local.master_address_by_environment[var.environment]
      master-port = "2135"
      database    = local.database_by_environment[var.environment]
      proto       = "pq"
      ssl = {
        enabled = 1
      }

      // Always read token from metadata instead of manually going to IAM
      iam = 1

      // FIXME: Remove when all targets will use IAM token from instance metadata 
      # iam-key-file = "/etc/yandex/statbox-push-client/key.json"
      # iam-endpoint = "iam.api.cloud.yandex.net"
    }

    logger = {
      remote = 0
      mode   = "stderr"
      level  = 6
    }

    watcher = {
      state = "/var/lib/push-client/"
    }

    files = [
      for file in var.files :
      {
        name  = file.path
        topic = file.topic
      }
    ]
  }

  configs = {
    "${local.config_folder}/${var.config_file}" = yamlencode(local.config)
  }
}