data "template_file" "collector_direct_podmanifest" {
  template = file("${path.module}/files/direct/pod.tpl.yaml")
  vars = {
    infra_pod_spec    = module.collector_direct_infrastructure_pod.infra-pod-spec
    config_digest     = sha256(data.template_file.collector_direct_configs.rendered)
    collector_version = var.ydb-plugin-version
  }
}

module "collector_direct_infrastructure_pod" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_collector_prod"
  metrics_agent_conf_path = "${path.module}/files/direct/metrics-agent.tpl.yaml"
}


data "template_file" "collector_direct_configs" {
  template = file("${path.module}/files/direct/configs.tpl.json")
}

resource "ycp_microcosm_instance_group_instance_group" "collector_direct_ig" {
  name        = "jaeger-collector-direct-ig-prod"
  description = "Jaeger collector installation (Direct YDB)"

  service_account_id = local.ig_sa

  scale_policy {
    fixed_scale {
      size = 0
    }
  }

  allocation_policy {
    dynamic "zone" {
      for_each = values(local.zones)
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_deleting    = 1
    max_unavailable = 1
    max_creating    = 1
    max_expansion   = 0
  }

  labels = {
    environment = local.environment
  }

  platform_l7_load_balancer_spec {
    preferred_ip_version = "IP_VERSION_UNSPECIFIED"
    target_group_spec {
      name = "jaeger-collector-direct-ig-tg-prod"
      address_names = [
        "ipv6",
      ]
    }
  }

  instance_template {
    service_account_id = local.instance_sa
    platform_id        = "standard-v2"

    resources {
      memory        = 12
      cores         = 12
      core_fraction = 100
    }

    scheduling_policy {
      preemptible = false
    }

    boot_disk {
      device_name = "boot"
      mode        = "READ_WRITE"
      disk_spec {
        size     = 20
        image_id = local.image_id
      }
    }

    network_interface {
      subnet_ids = local.subnets
      primary_v6_address {
        name = "ipv6"
      }
      primary_v4_address {
        name = "ipv4"
      }
    }

    labels = {
      environment     = "prod"
      env             = "prod"
      conductor-group = "jaeger"
      conductor-role  = "collector"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.collector_direct_dns_prefix
    }

    metadata = {
      internal-name     = local.collector_direct_dns_prefix
      internal-hostname = local.collector_direct_dns_prefix
      internal-fqdn     = local.collector_direct_dns_prefix
      yandex-dns        = local.collector_direct_dns_prefix
      docker-config     = data.template_file.docker_json.rendered
      environment       = "prod"
      podmanifest       = data.template_file.collector_direct_podmanifest.rendered
      infra-configs     = module.collector_direct_infrastructure_pod.infra-pod-configs
      configs           = data.template_file.collector_direct_configs.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}
