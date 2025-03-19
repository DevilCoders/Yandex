provider "yandex" {
  endpoint  = var.yc_endpoint
  token     = module.yc_token.result
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

locals {
  instances = [
    {
      ipv4 = "172.16.0.5"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb01"
    },
    {
      ipv4 = "172.17.0.10"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb02"
    },
    {
      ipv4 = "172.18.0.14"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb03"
    },
    {
      ipv4 = "172.16.0.23"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb04"
    },
    {
      ipv4 = "172.17.0.40"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb05"
    },
    {
      ipv4 = "172.18.0.23"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb06"
    },
    {
      ipv4 = "172.16.0.22"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb07"
    },
    {
      ipv4 = "172.17.0.34"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb08"
    },
    {
      ipv4 = "172.18.0.11"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb09"
    },
    {
      ipv4 = "172.16.0.12"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb10"
    },
    {
      ipv4 = "172.17.0.15"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb11"
    },
    {
      ipv4 = "172.18.0.39"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb12"
    },
    {
      ipv4 = "172.16.0.28"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb13"
    },
    {
      ipv4 = "172.17.0.6"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb14"
    },
    {
      ipv4 = "172.18.0.35"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb15"
    },
    {
      ipv4 = "172.16.0.37"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb16"
    },
    {
      ipv4 = "172.17.0.116"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb17"
    },
    {
      ipv4 = "172.18.0.135"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb18"
    },
    {
      ipv4 = "172.16.0.137"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb19"
    },
    {
      ipv4 = "172.17.0.106"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb20"
    },
    {
      ipv4 = "172.18.0.136"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb21"
    },
    {
      ipv4 = "172.16.0.106"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb22"
    },
    {
      ipv4 = "172.17.0.107"
      ipv6 = "2a02:6b8:c02:900:0:f805:0:eb23"
    },
    {
      ipv4 = "172.18.0.107"
      ipv6 = "2a02:6b8:c03:500:0:f805:0:eb24"
    },
    {
      ipv4 = "172.16.0.107"
      ipv6 = "2a02:6b8:c0e:500:0:f805:0:eb25"
  }]
}

module "oauth" {
  source = "../../modules/yav-oauth"
}

module "yc_token" {
  source = "../../modules/yc-token"
}

module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = module.oauth.result
  abc_service  = "cloud-platform"
}

data "yandex_compute_image" "this" {
  family    = "platform-teamcity-agent"
  folder_id = var.yc_folder
}

data "yandex_iam_service_account" "this" {
  # CLOUD-58707
  service_account_id = "ajeqd5kjndg7qnnosqhu"
}

resource "yandex_compute_instance" "agent" {
  count = var.instance_count

  service_account_id = data.yandex_iam_service_account.this.id

  platform_id = "standard-v2"
  name = element(
    data.template_file.teamcity-agent-name.*.rendered,
    count.index,
  )
  hostname = element(
    data.template_file.teamcity-agent-name.*.rendered,
    count.index,
  )
  description               = var.instance_description
  zone                      = element(var.zones, count.index)
  allow_stopping_for_update = true

  resources {
    cores         = var.instance_cores
    core_fraction = var.instance_core_fraction
    memory        = var.instance_memory
  }

  boot_disk {
    initialize_params {
      # image_id = data.yandex_compute_image.this.id

      image_id = "fd88qh915uhhoag2a7vd"
      size     = var.instance_disk_size
    }
  }

  network_interface {
    subnet_id    = var.zone_subnets[element(var.zones, count.index)]
    ipv6         = true
    ip_address   = lookup(local.instances[count.index], "ipv4")
    ipv6_address = lookup(local.instances[count.index], "ipv6")
  }

  metadata = {
    teamcity-seed = random_string.cluster-token.result
    ssh-keys      = module.ssh-keys.ssh-keys

    fqdn = "${element(
      data.template_file.teamcity-agent-name.*.rendered,
      count.index,
    )}.ycp.cloud.yandex.net"
    yandex-dns = element(
      data.template_file.teamcity-agent-name.*.rendered,
      count.index,
    )
    user-data = element(
      data.template_file.user-data.*.rendered,
      count.index,
    )
  }

  labels = {
    role                 = "dp-light-teamcity-agent"
    cluster_id           = random_string.cluster-token.result
    skip_update_ssh_keys = var.skip_update_ssh_keys
    yandex-dns = element(
      data.template_file.teamcity-agent-name.*.rendered,
      count.index,
    )
    conductor-group = "teamcity-agent"
    conductor-dc    = var.zone_suffix[element(var.zones, count.index)]
  }

  //  provisioner "local-exec" {
  //    command = "dns-monkey.pl --zone-update  --expression \"add ${element(
  //      data.template_file.teamcity-agent-name.*.rendered,
  //      count.index,
  //    )}.ycp.cloud.yandex.net ${self.network_interface[0].ipv6_address}\""
  //  }
  //
  //  provisioner "local-exec" {
  //    when = destroy
  //    command = "dns-monkey.pl --zone-update  --expression \"delete ${element(
  //      data.template_file.teamcity-agent-name.*.rendered,
  //      count.index,
  //    )}.ycp.cloud.yandex.net ${self.network_interface[0].ipv6_address}\""
  //  }
}

data "template_file" "teamcity-agent-name" {
  count    = var.instance_count
  template = "$${name}"

  vars = {
    name = "dp-light-tc-agent-${format("%02d", count.index + 1)}"
  }
}

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

data "template_file" "user-data" {
  count    = var.instance_count
  template = file("${path.module}/files/docker-json-cloud-config.txt")

  vars = {
    docker_auth                   = local.docker_auth
    docker_auth_cr_yandex         = local.docker_auth_cr_yandex
    docker_auth_cr_yandex_preprod = local.docker_auth_cr_yandex_preprod

    fqdn = "${element(
      data.template_file.teamcity-agent-name.*.rendered,
      count.index,
    )}.ycp.cloud.yandex.net"
  }
}

