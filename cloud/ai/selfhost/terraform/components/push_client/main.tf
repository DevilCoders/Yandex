locals {
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/push-client"
    staging = "cr.yandex/crppns4pq490jrka0sth/push-client"
    prod    = "cr.yandex/crppns4pq490jrka0sth/push-client"
  }

  latest_tag = "7535166"
  tag        = coalesce(var.tag, local.latest_tag)

  config_folder =  "/etc/yandex/statbox-push-client"
}

module "push_client_container" {
  source = "../../modules/container"

  name        = var.config_file == "push-client.yaml" ? "push-client" : "push-client-billing"
  environment = var.environment

  repositories = local.repositories
  tag          = local.tag

  command = [
    "/usr/bin/push-client",
    "-f",
    "-c",
    "${local.config_folder}/${var.config_file}"
  ]

  mounts = {
    var-log = {
      path                          = "/var/log"
      terminationGracePeriodSeconds = 30
      read_only                     = false
    }

    # TODO: Make this conditional or all location of logs configurable
    var-lib-billing = {
      path                          = "/var/lib/billing"
      terminationGracePeriodSeconds = 30
      read_only                     = false
    }
    var-spool = {
      path                          = "/var/spool"
      terminationGracePeriodSeconds = 30
      read_only                     = false
    }
    push-client-config = {
      path      = local.config_folder
      read_only = true
    }
    etc-secrets = {
      path      = "/etc/secrets"
      read_only = true
    }
  }
}