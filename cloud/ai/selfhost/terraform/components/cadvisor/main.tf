locals {
  repositories = {
    preprod = "cr.yandex/crppns4pq490jrka0sth/cadvisor"
    staging = "cr.yandex/crppns4pq490jrka0sth/cadvisor"
    prod    = "cr.yandex/crppns4pq490jrka0sth/cadvisor"
  }

  latest_tag = "v0.32.0"
  tag        = coalesce(var.tag, local.latest_tag)

  cadvisor_port = 17008
}

module "cadvisor_container" {
  source = "../../modules/container"

  name = "cadvisor"

  environment  = var.environment
  repositories = local.repositories
  tag          = local.tag

  ports = [
    local.cadvisor_port
  ]

  mounts = {
    root = {
      path      = "/rootfs"
      read_only = true
    }
    var-run = {
      path      = "/var/run"
      read_only = true
    }
    sys = {
      path      = "/sys"
      read_only = true
    }
    var-lib-docker = {
      path      = "/var/lib/docker"
      read_only = true
    }
    dev-disk = {
      path      = "/dev/disk"
      read_only = true
    }
    etc-secrets = {
      path      = "/etc/secrets"
      read_only = true
    }
  }

  command = [
    "/usr/bin/cadvisor",
    "-logtostderr",
    "--port=${local.cadvisor_port}"
  ]
}