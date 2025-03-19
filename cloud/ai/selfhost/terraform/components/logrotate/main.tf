locals {
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/logrotate"
    staging = "cr.yandex/crppns4pq490jrka0sth/logrotate"
    prod    = "cr.yandex/crppns4pq490jrka0sth/logrotate"
  }

  latest_tag = "0.1"
  tag        = coalesce(var.tag, local.latest_tag)
}

module "logrotate_container" {
  source = "../../modules/container"

  name = "logrotate"

  environment  = var.environment
  repositories = local.repositories
  tag          = local.tag

  mounts = {

    logrotate-config = {
      path      = "/etc/logrotate.d"
      read_only = true
    }

    etc-secrets = {
      path      = "/etc/secrets"
      read_only = true
    }

    var-log = {
      path      = "/var/log"
      read_only = false
    }

    var-lib-docker = {
      path      = "/var/lib/docker"
      read_only = false
    }
  }
}