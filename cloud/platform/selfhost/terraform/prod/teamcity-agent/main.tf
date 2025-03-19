provider "yandex" {
  endpoint  = var.yc_endpoint
  token     = var.yc_token
  folder_id = var.yc_folder
}

module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "cloud-platform"
}

variable "instance_core_fraction" {
  default = "100"
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

  platform_id = "standard-v2"

  service_account_id = data.yandex_iam_service_account.this.id

  name                      = "dp-tc-agent-${format("%02d", count.index + 1)}"
  hostname                  = "dp-tc-agent-${format("%02d", count.index + 1)}"
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
      image_id = "fd88qh915uhhoag2a7vd" # TODO: Latest image doesn't work, fix it.
      #image_id = data.yandex_compute_image.this.id
      size     = var.instance_disk_size
      type     = "network-ssd"
    }
  }

  network_interface {
    subnet_id    = var.zone_subnets[element(var.zones, count.index)]
    ipv4         = true
    ipv6         = true
    ipv6_address = element(var.ipv6_addrs, count.index)
  }

  metadata = {
    teamcity-seed = random_string.cluster-token.result
    ssh-keys      = module.ssh-keys.ssh-keys
    fqdn          = "dp-tc-agent-${format("%02d", count.index + 1)}.ycp.cloud.yandex.net"
    yandex-dns    = "dp-tc-agent-${format("%02d", count.index + 1)}"

    user-data = templatefile("${path.module}/files/docker-json-cloud-config.txt", {
      docker_auth                   = local.docker_auth
      docker_auth_cr_yandex         = local.docker_auth_cr_yandex
      docker_auth_cr_yandex_preprod = local.docker_auth_cr_yandex_preprod
      fqdn                          = "dp-tc-agent-${format("%02d", count.index + 1)}.ycp.cloud.yandex.net"
    })
  }

  labels = {
    role                 = "dp-teamcity-agent"
    cluster_id           = random_string.cluster-token.result
    skip_update_ssh_keys = var.skip_update_ssh_keys
    yandex-dns           = "dp-tc-agent-${format("%02d", count.index + 1)}"
    conductor-group      = "teamcity-agent"
    conductor-dc         = var.zone_suffix[element(var.zones, count.index)]
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

resource "random_string" "cluster-token" {
  length  = 16
  special = false
  upper   = false
}

