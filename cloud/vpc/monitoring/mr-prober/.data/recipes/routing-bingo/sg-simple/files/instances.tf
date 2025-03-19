resource "ycp_vpc_security_group" "egress" {
  folder_id = var.folder_id

  network_id  = ycp_vpc_network.network.id
  name        = "${var.prefix}-egress-sg"
  description = "Security group for agent (egress)"

  rule_spec {
    direction = "EGRESS"

    protocol_name = "TCP"
    cidr_blocks {
      v4_cidr_blocks = var.agent_rule.v4_cidr_blocks
      v6_cidr_blocks = var.agent_rule.v6_cidr_blocks
    }

    ports {
      from_port = var.agent_rule.from_port
      to_port   = var.agent_rule.to_port
    }
  }

  rule_spec {
    direction = "EGRESS"

    protocol_name = "ICMP"
    cidr_blocks {
      v4_cidr_blocks = var.target_rule.v4_cidr_blocks
    }
  }

  rule_spec {
    direction = "EGRESS"

    protocol_name = "IPV6_ICMP"
    cidr_blocks {
      v6_cidr_blocks = var.target_rule.v6_cidr_blocks
    }
  }
}

resource "ycp_vpc_security_group" "ingress" {
  folder_id = var.folder_id

  network_id  = ycp_vpc_network.network.id
  name        = "${var.prefix}-ingress-sg"
  description = "Security group for targets (ingress)"

  rule_spec {
    direction = "INGRESS"

    protocol_name = "TCP"
    cidr_blocks {
      v4_cidr_blocks = var.target_rule.v4_cidr_blocks
      v6_cidr_blocks = var.target_rule.v6_cidr_blocks
    }

    ports {
      from_port = var.target_rule.from_port
      to_port   = var.target_rule.to_port
    }
  }

  rule_spec {
    direction = "INGRESS"

    protocol_name = "ICMP"
    cidr_blocks {
      v4_cidr_blocks = var.target_rule.v4_cidr_blocks
    }
  }

  rule_spec {
    direction = "INGRESS"

    protocol_name = "IPV6_ICMP"
    cidr_blocks {
      v6_cidr_blocks = var.target_rule.v6_cidr_blocks
    }
  }
}

resource "ycp_compute_instance" "agent" {
  for_each = {
    for node in var.compute_nodes : node.fqdn => {
      zone          = node.zone
      compute_node  = node.fqdn
      name          = "${var.prefix}-${node.zone}-agent"
      internal_name = "agent"
      fqdn          = "agent-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
    } if node.has_agent
  }

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
    subnet_id          = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone), "id")
    security_group_ids = var.agent_has_sg ? [ycp_vpc_security_group.egress.id] : []
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
    skm           = data.external.skm_metadata.result.skm
    prober-expect = var.prober_expect
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}

resource "ycp_compute_instance" "target" {
  for_each = {
    for node in var.compute_nodes : node.fqdn => {
      zone          = node.zone
      compute_node  = node.fqdn
      suffix        = node.has_agent ? "local" : "remote"
      name          = "${var.prefix}-${node.zone}-target-${node.has_agent ? "local" : "remote"}"
      internal_name = "target-${node.has_agent ? "local" : "remote"}"
      fqdn          = "target-${node.has_agent ? "local" : "remote"}-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
    }
  }

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.name
  fqdn        = each.value.fqdn
  description = "Target ${each.value.suffix} for zone ${each.value.zone}"

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
      image_id = ycp_compute_image.web_server.id
    }
  }

  # Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  network_interface {
    subnet_id          = lookup(lookup(ycp_vpc_subnet.subnets, each.value.zone), "id")
    security_group_ids = var.target_has_sg ? [ycp_vpc_security_group.ingress.id] : []
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
        stand_name                     = local.mr_prober_environment,,
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

resource "ytr_conductor_host" "agent" {
  for_each = {
    for node in var.compute_nodes : node.fqdn => {
      zone = node.zone
      fqdn = "agent.${node.zone}.${var.prefix}.${var.dns_zone}"
    } if node.has_agent
  }

  fqdn          = each.value.fqdn
  short_name    = each.value.fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, each.value.zone)
  group_id      = ytr_conductor_group.cluster.id
}

resource "ytr_conductor_host" "target" {
  for_each = {
    for node in var.compute_nodes : node.fqdn => {
      zone = node.zone
      fqdn = "target-${node.has_agent ? "local" : "remote"}.${node.zone}.${var.prefix}.${var.dns_zone}"
    }
  }

  fqdn          = each.value.fqdn
  short_name    = each.value.fqdn
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, each.value.zone)
  group_id      = ytr_conductor_group.cluster.id
}
