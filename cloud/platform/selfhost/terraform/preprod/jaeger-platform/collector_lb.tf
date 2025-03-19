data "template_file" "collector_lb_podmanifest" {
  template = file("${path.module}/files/lb/pod.tpl.yaml")
  vars = {
    infra_pod_spec    = module.collector_lb_infrastructure_pod.infra-pod-spec
    config_digest     = sha256(data.template_file.collector_lb_configs.rendered)
    collector_version = var.ydb-plugin-version
    plugin_version    = var.lb-plugin-version
  }
}

module "collector_lb_infrastructure_pod" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_collector_preprod"
  metrics_agent_conf_path = "${path.module}/files/lb/metrics-agent.tpl.yaml"
}

data "template_file" "collector_lb_configs" {
  template = file("${path.module}/files/lb/configs.tpl.json")

  vars = {
    jaeger_lb_plugin_config = data.template_file.collector_lb_config.rendered
  }
}

data "template_file" "collector_lb_config" {
  template = file("${path.module}/files/lb/config.tpl.yaml")
  vars = {
    token = module.jaeger-lb-token.value
  }
}

resource "ycp_microcosm_instance_group_instance_group" "collector_lb_ig" {
  name        = "jaeger-collector-lb-ig-preprod"
  description = "Jaeger collector installation (LB L7)"

  service_account_id = "bfbkk5if54u6qm319u0e"

  scale_policy {
    fixed_scale {
      size = 3
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
    environment = "preprod"
  }

  platform_l7_load_balancer_spec {
    preferred_ip_version = "IP_VERSION_UNSPECIFIED"
    target_group_spec {
      name = "jaeger-collector-lb-ig-tg-preprod"
      address_names = [
        "ipv6",
      ]
    }
  }

  instance_template {
    service_account_id = "bfblesupq182q80mpq26"
    platform_id        = "standard-v2"

    resources {
      memory        = 2
      cores         = 2
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
      subnet_ids = [
        "bucpba0hulgrkgpd58qp",
        "bltueujt22oqg5fod2se",
        "fo27jfhs8sfn4u51ak2s"
      ]
      primary_v6_address {
        name = "ipv6"
      }
      primary_v4_address {
        name = "ipv4"
      }
    }

    labels = {
      environment     = "preprod"
      env             = "pre-prod"
      conductor-group = "jaeger"
      conductor-role  = "collector"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.collector_lb_dns_prefix
    }

    metadata = {
      internal-name     = local.collector_lb_dns_prefix
      internal-hostname = local.collector_lb_dns_prefix
      internal-fqdn     = local.collector_lb_dns_prefix
      yandex-dns        = local.collector_lb_dns_prefix
      docker-config     = data.template_file.docker_json.rendered
      environment       = "preprod"
      podmanifest       = data.template_file.collector_lb_podmanifest.rendered
      infra-configs     = module.collector_lb_infrastructure_pod.infra-pod-configs
      configs           = data.template_file.collector_lb_configs.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}
