resource "yandex_compute_image" "agent" {
  folder_id = var.folder_id

  name        = "${var.prefix}-agent"
  description = "Image for agent built with packer from Mr.Prober images collection"
  source_url  = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/fd81qruuv96qaek976qq.qcow2"

  timeouts {
    create = "10m"
  }
}

resource "ycp_vpc_network" "network" {
  folder_id = var.folder_id

  name        = "${var.prefix}-network"
  description = "Network for cluster"
}

resource "ycp_vpc_subnet" "subnets" {
  for_each = toset(var.zones)

  folder_id  = var.folder_id
  network_id = ycp_vpc_network.network.id

  name    = format("%s-network-%s", var.prefix, each.key)
  zone_id = each.key

  egress_nat_enable = true

  v4_cidr_blocks = [var.network_ipv4_cidrs[each.key]]
}


resource "yandex_vpc_security_group" "instances" {
  folder_id = var.folder_id

  network_id  = ycp_vpc_network.network.id
  name        = "instances-sg"
  description = "Security group for instances"

  ingress {
    protocol       = "TCP"
    description    = "Allow SSH"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port           = 22
  }

  ingress {
    protocol       = "ICMP"
    description    = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port           = "0"
  }

  ingress {
    protocol       = "IPV6_ICMP"
    description    = "Allow pings"
    v6_cidr_blocks = ["::/0"]
    port           = "0"
  }

  egress {
    protocol       = "ANY"
    description    = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port      = 0
    to_port        = 65535
  }
}

data "external" "skm_metadata" {
  program = [
    "python3", "${path.module}/generate.py"
  ]
  query   = {
    secrets_file = "${abspath(path.module)}/secrets.yaml",

    cache_dir   = "/tmp/.skm-cache/mr-prober/",
    key_uri     = "yc-kms://${var.mr_prober_secret_kek_id}"
  }
}

locals {
  host_prefix_to_zone_mapping = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
    spb = "ru-gpn-spb99"
    # TODO: private-testing isn't supported now, because private-testing has sas* compute_nodes, same as the ru-central1-b zone.
    # But in private-tesing's Mr.Prober cluster we can just omit other values in this mapping.
  }
}

resource "ycp_compute_instance" "instance" {
  for_each = {
  for node in var.compute_nodes : node => {
    zone     = lookup(local.host_prefix_to_zone_mapping, substr(node, 0, 3))
    hostname = "${element(split(".", node), 0)}.${var.prefix}"
  }
  }

  folder_id = var.folder_id

  name        = each.value.hostname
  hostname    = each.value.hostname
  description = "Instance on ${each.key}"

  platform_id = "e2e"
  zone_id     = each.value.zone

  resources {
    memory        = 1
    cores         = 2
    core_fraction = 5
  }

  boot_disk {
    disk_spec {
      image_id = yandex_compute_image.agent.id
    }
  }

  network_interface {
    subnet_id          = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone), "id")
    security_group_ids = [yandex_vpc_security_group.instances.id]
    primary_v4_address {
    }
  }

  network_interface {
    subnet_id = lookup(var.control_network_subnet_ids, each.value.zone)
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn        = "${each.value.hostname}.${var.dns_zone}."
        ptr         = true
      }
    }
  }

  labels = {
    abc_svc = "ycvpc"
    layer   = "iaas"
    env     = var.label_environment
  }

  metadata = {
    user-data = templatefile(
    "${path.module}/cloud-init.yaml",
    {
      hostname   = "${each.value.hostname}.${var.dns_zone}",
      stand_name = var.yc_profile,
      cluster_id = var.cluster_id
    }
    )
    skm       = data.external.skm_metadata.result.skm
  }

  placement_policy {
    compute_nodes = [each.key]
    host_group    = "e2e"
  }

  scheduling_policy {
    service = true
  }

  service_account_id = var.mr_prober_sa_id

  allow_stopping_for_update = true
}

resource "ytr_conductor_host" "instance" {
  for_each = {
  for node in var.compute_nodes : node => {
    datacenter = substr(node, 0, 3)
    hostname   = "${element(split(".", node), 0)}.${var.prefix}"
  }
  }

  fqdn          = "${each.value.hostname}.${var.dns_zone}"
  short_name    = "${each.value.hostname}.${var.dns_zone}"
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_datacenter, each.value.datacenter)
  group_id      = ytr_conductor_group.meeseeks_in_datacenter[each.value.datacenter].id
}

