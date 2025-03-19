locals {
  agent_host_base = 3

  agents = {
    for index, node in var.compute_nodes : node.fqdn => {
      zone          = node.zone
      compute_node  = node.fqdn
      name          = "${var.prefix}-${node.zone}-agent"
      internal_name = "agent"
      fqdn          = "agent-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
      route         = module.left.worker_cidrs[index]
    } if node.has_agent
  }

  agent_inner_v4_address = module.left.worker_cidrs[0].v4_inner_address
  agent_inner_v6_address = module.left.worker_cidrs[0].v6_inner_address

  workers = merge(
    {
      for index, node in var.compute_nodes : (node.has_agent ? "local" : "remote") => {
        zone          = node.zone
        compute_nodes = [node.fqdn]
        name          = "${var.prefix}-${node.zone}-worker-left-${node.has_agent ? "local" : "remote"}"
        internal_name = "worker-${node.has_agent ? "local" : "remote"}"
        fqdn          = "worker-${node.has_agent ? "local" : "remote"}-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
        subnet_id     = lookup(lookup(module.left.subnets, node.zone), "id")
        route         = module.left.worker_cidrs[index + length(local.agents)]
      }
    },
    {
      right = {
        zone          = var.right_zone_id
        compute_nodes = []
        name          = "${var.prefix}-${var.right_zone_id}-worker-right"
        internal_name = "worker-right"
        fqdn          = "worker-right-${lookup(local.dc_short, var.right_zone_id, var.right_zone_id)}.${var.prefix}.${var.dns_zone}"
        subnet_id     = lookup(lookup(module.right.subnets, var.right_zone_id), "id")
        route         = module.right.worker_cidrs[0]
      }
  })

  routers = {
    for index, node in var.compute_nodes : (node.has_agent ? "local" : "remote") => {
      index         = index
      zone          = node.zone
      compute_nodes = [node.fqdn]
      name          = "${var.prefix}-${node.zone}-router-${node.has_agent ? "local" : "remote"}"
      internal_name = "router-${node.has_agent ? "local" : "remote"}"
      fqdn          = "router-${node.has_agent ? "local" : "remote"}-${lookup(local.dc_short, node.zone, node.zone)}.${var.prefix}.${var.dns_zone}"
    }
  }

  router_devs = {
    left  = "eth1"
    right = "eth2"
  }
}

resource "ycp_compute_image" "router" {
  folder_id = var.folder_id

  name        = "${var.prefix}-router"
  description = "Base ubuntu image for building routers"
  uri         = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/router/fd8va039flgkra6ols0s.qcow2"

  os_type       = "LINUX"
  min_disk_size = 10
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
    subnet_id = lookup(lookup(module.left.subnets, each.value.zone), "id")
    primary_v4_address {
      address = each.value.route.v4_address
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
    primary_v6_address {
      address = each.value.route.v6_address
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
        bootcmd = [
          "ip addr add ${local.agent_inner_v4_address} dev eth0:4",
          "ip route add ${local.v4_inner_cidr} via ${each.value.route.v4_default_router} src ${local.agent_inner_v4_address}",

          "ip addr add ${local.agent_inner_v6_address} dev eth0:6",
          "ip route add ${local.v6_inner_cidr} via ${each.value.route.v6_default_router} src ${local.agent_inner_v6_address}",
        ]
      }
    )
    skm = local.skm_metadata

    rb-targets = jsonencode({
      for key, worker in local.workers : key => {
        inet  = worker.route.v4_inner_address
        inet6 = "[${worker.route.v6_inner_address}]"
      }
    })
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}

resource "ycp_compute_instance" "workers" {
  for_each = local.workers

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.name
  fqdn        = each.value.fqdn
  description = "Worker ${each.key} in zone ${each.value.zone}"

  zone_id = each.value.zone
  placement_policy {
    compute_nodes = each.value.compute_nodes
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
    subnet_id = each.value.subnet_id
    primary_v4_address {
      address = each.value.route.v4_address
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
    primary_v6_address {
      address = each.value.route.v6_address
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
        agent_additional_metric_labels = var.agent_additional_metric_labels,
        s3_endpoint                    = var.s3_endpoint,
        runcmd                         = [],
        bootcmd = [
          "ip addr add ${each.value.route.v4_inner_address} dev eth0:4",
          "ip route add ${local.v4_inner_cidr} via ${each.value.route.v4_default_router} src ${each.value.route.v4_inner_address}",

          "ip addr add ${each.value.route.v6_inner_address} dev eth0:6",
          "ip route add ${local.v6_inner_cidr} via ${each.value.route.v6_default_router} src ${each.value.route.v6_inner_address}",
        ]
      }
    )
    skm = local.skm_metadata
  }

  service_account_id        = var.mr_prober_sa_id
  allow_stopping_for_update = true
}


resource "ycp_compute_instance" "routers" {
  for_each = local.routers

  folder_id = var.folder_id

  name        = each.value.name
  hostname    = each.value.name
  fqdn        = each.value.fqdn
  description = "${title(each.key)} router in zone ${each.value.zone}"

  zone_id = each.value.zone
  placement_policy {
    compute_nodes = each.value.compute_nodes
  }

  platform_id = var.compute_platform_id
  resources {
    cores         = 2
    core_fraction = 5
    memory        = 1
  }

  boot_disk {
    disk_spec {
      image_id = ycp_compute_image.router.id
    }
  }

  // Just a placeholder. See https://st.yandex-team.ru/CLOUD-97056
  gpu_settings {}

  // Note: router uses eth0 as control interface
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

  network_interface {
    subnet_id = lookup(lookup(module.left.subnets, each.value.zone), "id")
    primary_v4_address {
      address = module.left.router_instance_ipv4_addrs[each.value.index]
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
    primary_v6_address {
      address = module.left.router_instance_ipv6_addrs[each.value.index]
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
  }

  network_interface {
    subnet_id = lookup(lookup(module.right.subnets, each.value.zone), "id")
    primary_v4_address {
      address = module.right.router_instance_ipv4_addrs[each.value.index]
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
      }
    }
    primary_v6_address {
      address = module.right.router_instance_ipv6_addrs[each.value.index]
      dns_record {
        fqdn = "${each.value.internal_name}.ru-central1.internal."
        ptr  = true
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
        bootcmd = [
          "ip route add ${local.v4_left_inner_cidr} dev ${local.router_devs.left}",
          "ip route add ${local.v6_left_inner_cidr} dev ${local.router_devs.left}",
          "ip route add ${local.v4_right_inner_cidr} dev ${local.router_devs.right}",
          "ip route add ${local.v6_right_inner_cidr} dev ${local.router_devs.right}",
        ]
      }
    )
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
