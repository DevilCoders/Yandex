// export YC_TOKEN=$(yc --profile=preprod iam create-token)
// terraform init -backend-config="secret_key=$(ya vault get version ver-01e9d91yf5qmcq891em025rwgr -o s3_secret_access_key 2>/dev/null)"
// terraform plan

locals {
  dns_zone_id = "aet5iocohvngcfg61d6a"
  dns_zone    = "on.cloud-preprod.yandex.net"
  folder_id   = data.terraform_remote_state.preprod-bootstrap-resources.outputs.yc-cloudvm-folder-id
  network_id  = "c645jru96isfvfclus76"
  subnet_ids  = data.terraform_remote_state.preprod-bootstrap-resources.outputs.yc-ci-subnet-ids
  image_id    = var.boot_disk_id
  alb_name    = "load-cloud-71275-l7"
}


data "terraform_remote_state" "preprod-bootstrap-resources" {
  backend = "http"

  config = {
    address = "https://s3.mds.yandex.net/yc-bootstrap-lts/terraform/preprod/bootstrap"
  }
}

data "ycp_compute_image" "build-agent-image" {
  image_id  = local.image_id
  folder_id = "service-images"
}

resource "ycp_compute_instance" "overlay_vms" {
  for_each        = var.vms
  name            = "${each.key}-selfhost-overlay"
  description     = "CLOUD-71275"
  zone_id         = "ru-central1-a"
  cauth           = true
  folder_id       = local.folder_id
  platform_id     = "standard-v2"
  pci_topology_id = "V2"
  labels          = {}

  resources {
    cores      = 4
    memory     = 8
    gpus       = 0
    nvme_disks = 0
  }

  metadata = {
    user-data = templatefile("${path.module}/user-data.tmpl", {
      fqdn : "${each.key}-selfhost-overlay.${local.dns_zone}"
      selfdns : var.selfdns_token
    })
    enable-oslogin = true
  }

  boot_disk {
    mode = "READ_WRITE"
    disk_spec {
      size     = 150
      labels   = {}
      image_id = data.ycp_compute_image.build-agent-image.id
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id          = local.subnet_ids["ru-central1-a"].id
    security_group_ids = []
    primary_v4_address {}
    primary_v6_address {}
  }

  lifecycle {
    ignore_changes = [
      boot_disk[0].disk_spec[0].labels,
    ]
  }
}

resource "ycp_compute_instance" "underlay_vms" {
  for_each        = var.vms
  name            = "${each.key}-selfhost-underlay"
  fqdn            = "${each.key}-selfhost-underlay.${local.dns_zone}"
  description     = "CLOUD-71275"
  zone_id         = "ru-central1-a"
  cauth           = true
  folder_id       = local.folder_id
  platform_id     = "standard-v2"
  pci_topology_id = "V2"
  labels          = {}

  resources {
    cores      = 4
    memory     = 8
    gpus       = 0
    nvme_disks = 0
  }

  metadata = {
    user-data = templatefile("${path.module}/user-data.tmpl", {
      fqdn : "${each.key}-selfhost-underlay.${local.dns_zone}"
      selfdns : var.selfdns_token
    })
    enable-oslogin = true
  }

  boot_disk {
    mode = "READ_WRITE"
    disk_spec {
      size     = 150
      labels   = {}
      image_id = data.ycp_compute_image.build-agent-image.id
      type_id  = "network-hdd"
    }
  }

  underlay_network {
    network_name = "underlay-v6"
  }

  lifecycle {
    ignore_changes = [
      boot_disk[0].disk_spec[0].labels,
    ]
  }
}

resource "ycp_dns_dns_record_set" "target-underlay-l3tt" {
  zone_id = local.dns_zone_id
  name    = "load-selfhost-l3tt"
  type    = "AAAA"
  ttl     = "3600"
  data    = ["2a02:6b8:0:3400:0:d12:0:7"]
}
