locals {
  nat_instances = {
    for node in var.compute_nodes : (node.has_agent ? "local" : "remote") => {
      zone          = node.zone
      compute_node  = node.fqdn
      suffix        = node.has_agent ? "local" : "remote"
      name          = "${var.prefix}-${node.zone}-nat-instance-${node.has_agent ? "local" : "remote"}"
      internal_name = "nat-instance-${node.has_agent ? "local" : "remote"}"
      fqdn          = "nat-instance-${node.has_agent ? "local" : "remote"}-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
    }
  }

  agents = {
    for node in var.compute_nodes : node.fqdn => {
      zone          = node.zone
      compute_node  = node.fqdn
      name          = "${var.prefix}-${node.zone}-agent"
      internal_name = "agent"
      fqdn          = "agent-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
    } if node.has_agent
  }
}

resource "ycp_compute_image" "nat_instance" {
  folder_id = var.folder_id

  // TODO: use automatically updating marketplace image
  name        = "${var.prefix}-nat-instance"
  description = "Image for NAT instance copied manually from Marketplace"
  uri         = "https://storage.yandexcloud.net/yc-vpc-packer-export/nat-instance.qcow2"

  os_type       = "LINUX"
  min_disk_size = 10
}

resource "ycp_vpc_address" "nat_instance" {
  for_each = local.nat_instances

  folder_id = var.folder_id
  name      = "${var.prefix}-nat-instance-${each.key}-address"

  external_ipv4_address_spec {
    zone_id = each.value.zone
    requirements {
      hints = var.external_address_hints
    }
  }

  lifecycle {
    ignore_changes = [reserved]
  }
}

resource "ycp_compute_instance" "nat_instance" {
  for_each = local.nat_instances

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.name
  fqdn        = each.value.fqdn
  description = "NAT instance ${each.value.suffix} for zone ${each.value.zone}"

  zone_id = each.value.zone
  placement_policy {
    compute_nodes = [each.value.compute_node]
  }

  platform_id = var.compute_platform_id
  resources {
    cores         = 2
    core_fraction = 5
    memory        = 1
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.nat_instance.id
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id = lookup(lookup(ycp_vpc_subnet.nat_instance_subnets, each.value.zone), "id")
    primary_v4_address {
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
      one_to_one_nat {
        address_id = ycp_vpc_address.nat_instance[each.value.suffix].id
        ip_version = "IPV4"
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
        hostname                       = each.value.fqdn,
        stand_name                     = local.mr_prober_environment,
        cluster_id                     = var.cluster_id,
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint                    = var.s3_endpoint,
        runcmd                         = [],
        bootcmd                        = []
      }
    )
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}

resource "ycp_vpc_security_group" "target" {
  folder_id = var.folder_id

  network_id  = ycp_vpc_network.target_network.id
  name        = "${var.prefix}-target-sg"
  description = "Security group for targets"

  rule_spec {
    direction     = "INGRESS"
    protocol_name = "ICMP"
    description   = "Allow pings"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
    }
  }

  rule_spec {
    direction     = "INGRESS"
    protocol_name = "UDP"
    description   = "Allow UDP-traceroute probe packets"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
    }
    ports {
      from_port = 33434
      to_port   = 33534
    }
  }

  rule_spec {
    direction     = "INGRESS"
    protocol_name = "TCP"
    description   = "Allow test connections"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
    }
    ports {
      from_port = 80
      to_port   = 80
    }
  }

  rule_spec {
    direction     = "EGRESS"
    protocol_name = "ANY"
    description   = "Allow any egress traffic"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
      v6_cidr_blocks = ["::/0"]
    }
    ports {
      from_port = 0
      to_port   = 65535
    }
  }
}

resource "ycp_vpc_address" "target" {
  count = 2

  folder_id = var.folder_id
  name      = "${var.prefix}-target-address-${count.index}"

  external_ipv4_address_spec {
    zone_id = var.target_zone_id
    requirements {
      hints = var.external_address_hints
    }
  }

  lifecycle {
    ignore_changes = [reserved]
  }
}

resource "ycp_compute_instance" "target" {
  count = 2

  folder_id = var.folder_id

  name        = "${var.prefix}-target-${count.index}"
  fqdn        = "target-${count.index}.${var.prefix}.${var.dns_zone}"
  description = "Target ${count.index} ${count.index}"

  zone_id = var.target_zone_id

  platform_id = var.compute_platform_id
  resources {
    cores         = 2
    core_fraction = 5
    memory        = 1
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.web_server.id
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id          = lookup(lookup(ycp_vpc_subnet.target_subnets, var.target_zone_id), "id")
    security_group_ids = [ycp_vpc_security_group.target.id]

    primary_v4_address {
      dns_record {
        fqdn = "target-${count.index}.ru-central1.internal."
        ptr  = true
      }

      one_to_one_nat {
        address_id = ycp_vpc_address.target[count.index].id
        ip_version = "IPV4"
      }
    }

    primary_v6_address {
      dns_record {
        fqdn = "target-${count.index}.ru-central1.internal."
        ptr  = true
      }
    }
  }

  network_interface {
    subnet_id = lookup(var.control_network_subnet_ids, var.target_zone_id)
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn        = "target-${count.index}.${var.prefix}.${var.dns_zone}."
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
        hostname                       = "target-${count.index}.${var.prefix}.${var.dns_zone}",
        stand_name                     = local.mr_prober_environment,
        cluster_id                     = var.cluster_id,
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint                    = var.s3_endpoint,
        runcmd                         = [],
        bootcmd                        = []
      }
    )
    skm = local.skm_metadata
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}

resource "ycp_compute_instance" "agent" {
  for_each = local.agents

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.name
  fqdn        = each.value.fqdn
  description = "Agent for zone ${each.value.zone}"

  zone_id = each.value.zone
  placement_policy {
    compute_nodes = [each.value.compute_node]
  }

  platform_id = var.compute_platform_id
  resources {
    cores         = 2
    core_fraction = 20
    memory        = 1
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.agent.id
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id = lookup(lookup(ycp_vpc_subnet.agent_subnets, each.value.zone), "id")
    primary_v4_address {
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
    primary_v6_address {
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
  }

  network_interface {
    subnet_id = lookup(var.control_network_subnet_ids, each.value.zone)
    primary_v6_address {
      dns_record {
        dns_zone_id = var.dns_zone_id
        fqdn        = "${each.value.fqdn}."
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
        hostname                       = each.value.fqdn,
        stand_name                     = local.mr_prober_environment,
        cluster_id                     = var.cluster_id,
        agent_additional_metric_labels = "{}",
        s3_endpoint                    = var.s3_endpoint,
        runcmd                         = [],
        bootcmd                        = []
      }
    )
    expected-nexthops = jsonencode(var.expected_nexthops)
    // FIXME(CLOUD-63662): rework this using DNS for one-to-one NAT
    rb-targets = jsonencode({
      for index, instance in ycp_compute_instance.target :
      "target${index}" => {
        inet = instance.network_interface[0].primary_v4_address[0].one_to_one_nat[0].address
      }
    })
    skm = local.skm_metadata
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}

resource "ytr_conductor_host" "agent" {
  for_each = local.agents

  fqdn          = each.value.fqdn
  short_name    = each.value.fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, each.value.zone)
  group_id      = ytr_conductor_group.cluster.id
}
