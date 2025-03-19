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

  # Rack name is extracted from compute node. Example of compute node hostname:
  #   vla04-st7-6.cloud.yandex.net (rack is vla04-st7)
  #   il1-a-ct4-26b.infra.yandexcloud.co.il (rack is il1-a-ct4)
  #
  # We use regular expression which matches all parts of the compute node's prefix, expect the last one.
  racks = toset([
    for node in var.compute_nodes: element(regex("^([-a-z0-9]*)-[a-z0-9]+\\.", node), 0)
  ])
}

resource "ycp_microcosm_instance_group_instance_group" "agents" {
  for_each = {
    for rack in local.racks: rack => {
      rack = rack
      # In ru-central1 compute node hostnames start with "vla", "sas", "myt", and not with the name of the AZ,
      # so we look at hard-coded host_prefix_to_zone_mapping here. But in other regions we hope that hostnames
      # will start with region name, so just extract it with one more regular expression.
      zone = lookup(
        local.host_prefix_to_zone_mapping,
        substr(rack, 0, 3),
        element(regex("^([-a-z0-9]*)-[a-z0-9]+$", rack), 0)
      )
      # Filter out compute nodes belonged to this rack
      nodes = sort([for node in var.compute_nodes: node if substr(node, 0, length(rack) + 1) == "${rack}-"])
    }
  }

  folder_id = var.folder_id

  name = "${var.prefix}-${each.value.rack}-agents"
  description = "Group of instances for running tests on rack ${each.value.rack}"

  service_account_id = var.mr_prober_sa_id

  allocation_policy {
    zone {
      zone_id = each.value.zone
      instance_tags_pool = each.value.nodes
    }
  }

  deploy_policy {
    max_unavailable = 1
    max_creating = 10
    max_expansion = 0
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = length(each.value.nodes)
    }
  }

  instance_template {
    name = "${var.prefix}-{meeseeks_hostname_{instance.tag}}"
    hostname = "{meeseeks_fqdn_{instance.tag}}"
    fqdn = "{meeseeks_fqdn_{instance.tag}}"
    description = "Meeseeks instance"

    platform_id = "e2e"

    service_account_id = var.mr_prober_sa_id

    resources {
      memory = 1
      cores = 2
      core_fraction = 5
    }

    boot_disk {
      disk_spec {
        image_id = ycp_compute_image.agent.id
        size = 10
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

    labels = {
      abc_svc = "ycvpc"
      layer = "iaas"
      env = var.label_environment
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for subnet in ycp_vpc_subnet.subnets: subnet.id]
      security_group_ids = [ycp_vpc_security_group.instances.id]
      primary_v4_address {
        dns_record_spec {
          fqdn = "{meeseeks_hostname_{instance.tag}}.ru-central1.internal."
          ptr = true
        }
      }
      primary_v6_address {
      }
    }

    network_interface {
      network_id = var.control_network_id
      subnet_ids = values(var.control_network_subnet_ids)
      primary_v6_address {
        dns_record_spec {
          dns_zone_id = var.dns_zone_id
          fqdn = "{meeseeks_fqdn_{instance.tag}}."
          ptr = true
        }
      }
    }

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "{meeseeks_fqdn_{instance.tag}}",
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

      # See https://wiki.yandex-team.ru/cloud/devel/instance-group/internal/
      # Skm keys are regenerated on Creator restart, so ignore them.
      # If you want update secrets for agents, add some new metadata key (i.e. "secrets_versions = 1.0")
      internal-metadata-live-update-keys = "internal-metadata-live-update-keys,skm,mr-prober-agent-docker-image"
    }

    placement_policy {
      compute_nodes = ["{instance.tag}"]
      host_group = "e2e"
    }

    scheduling_policy {
      service = var.use_service_slot
    }
  }

  dynamic "variable" {
    for_each = each.value.nodes
    content {
      key = "meeseeks_fqdn_${variable.value}"
      value = "${element(split(".", variable.value), 0)}.${var.prefix}.${var.dns_zone}"
    }
  }

  dynamic "variable" {
    for_each = each.value.nodes
    content {
      key = "meeseeks_hostname_${variable.value}"
      value = element(split(".", variable.value), 0)
    }
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
