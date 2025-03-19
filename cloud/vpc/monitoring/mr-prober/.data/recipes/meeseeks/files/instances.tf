resource "ycp_compute_image" "agent" {
  folder_id = var.folder_id

  name = "${var.prefix}-agent"
  description = "Image for agent built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/${var.mr_prober_agent_image_name}.qcow2"

  os_type = "LINUX"
  min_disk_size = 10

  pooled = true
}

resource "ycp_vpc_network" "network" {
  folder_id   = var.folder_id

  name        = "${var.prefix}-network"
  description = "Network for cluster"
}

resource "ycp_vpc_subnet" "subnets" {
  for_each       = toset(var.zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("%s-network-%s", var.prefix, each.key)
  zone_id        = each.key

  egress_nat_enable = true

  v4_cidr_blocks = [var.monitoring_network_ipv4_cidrs[each.key]]

  v6_cidr_blocks = [var.monitoring_network_ipv6_cidrs[each.key]]
  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.monitoring_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}

resource "ycp_vpc_security_group" "instances" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.network.id
  name = "${var.prefix}-instances-sg"
  description = "Security group for instances"

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow SSH"
    cidr_blocks {
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/48"]
    }
    ports {
      from_port = 22
      to_port = 22
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "ICMP"
    description = "Allow pings"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "IPV6_ICMP"
    description = "Allow pings"
    cidr_blocks {
      v6_cidr_blocks = ["::/0"]
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow solomon-agent port for backward compatible access from compute-nodes via both interfaces: monitoring and control"
    cidr_blocks {
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/48"]
    }
    ports {
      from_port = 8080
      to_port = 8080
    }
  }

  rule_spec {
    direction = "EGRESS"
    protocol_name = "ANY"
    description = "Allow any egress traffic"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
      v6_cidr_blocks = ["::/0"]
    }
    ports {
      from_port = 0
      to_port = 65535
    }
  }
}

locals {
  host_prefix_to_zone_mapping = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
  }
}

resource "ycp_compute_instance" "instance" {
  for_each = {
    for node in var.compute_nodes : node => {
      zone = lookup(local.host_prefix_to_zone_mapping, substr(node, 0, 3))
      name = "${var.prefix}-${element(split(".", node), 0)}"
      hostname = element(split(".", node), 0)
      fqdn = "${element(split(".", node), 0)}.${var.prefix}.${var.dns_zone}"
    }
  }

  folder_id = var.folder_id

  name = each.value.name
  hostname = each.value.fqdn
  fqdn = each.value.fqdn
  description = "Instance on ${each.key}"

  platform_id = "e2e"
  zone_id = each.value.zone

  resources {
    memory = 1
    cores = 2
    core_fraction = 5
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.agent.id
    }
  }

  # Disk for NBS probers
  secondary_disk {
    disk_spec {
      description = "Disk for monitoring tests only"
      type_id     = "network-ssd"
      size        = 1
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone), "id")
    security_group_ids = [ycp_vpc_security_group.instances.id]
    primary_v4_address {
      dns_record {
        fqdn = "${each.value.hostname}.ru-central1.internal."
        ptr = true
      }
    }
    primary_v6_address {
    }
  }

  network_interface {
    subnet_id = lookup(var.control_network_subnet_ids, each.value.zone)
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn = "${each.value.fqdn}."
        ptr = true
      }
    }
  }

  labels = {
    abc_svc = "ycvpc"
    layer = "iaas"
    env = var.label_environment
  }

  metadata = {
    user-data = templatefile(
      "${path.module}/cloud-init.yaml",
      {
        hostname = each.value.fqdn,
        stand_name = local.mr_prober_environment,
        cluster_id = var.cluster_id,
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint = var.s3_endpoint,
        runcmd = [],
        bootcmd = [
          "parted -a optimal /dev/vdb mklabel msdos",
          "parted -a optimal /dev/vdb mkpart primary 0% 100%",
          "mkfs.ext4 /dev/vdb1",
          "mkdir /mnt/test",
          "mount /dev/vdb1 /mnt/test"
        ]
      }
    )
    skm = local.skm_metadata
    mr-prober-agent-docker-image = "${var.mr_prober_container_registry}/agent:${var.mr_prober_agent_docker_image_version}"
  }

  placement_policy {
    compute_nodes = [each.key]
    host_group = "e2e"
  }

  scheduling_policy {
    service = var.use_service_slot
  }

  service_account_id = var.mr_prober_sa_id

  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      local_disk, # https://st.yandex-team.ru/CLOUD-91252#6207cce8f756102386d9107d
      secondary_disk # https://st.yandex-team.ru/CLOUD-99644
    ]
  }
}

resource "ytr_conductor_host" "instance" {
  for_each = {
    for node in (var.use_conductor ? var.compute_nodes : []): node => {
      datacenter = substr(node, 0, 3)
      fqdn = "${element(split(".", node), 0)}.${var.prefix}.${var.dns_zone}"
    }
  }

  fqdn = each.value.fqdn
  short_name = each.value.fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_datacenter, each.value.datacenter)
  group_id = ytr_conductor_group.meeseeks_in_datacenter[each.value.datacenter].id
}
