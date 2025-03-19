data "template_file" "query_ydb2_podmanifest" {
  template = file("${path.module}/files/query-ydb2/pod.tpl.yaml")
  vars = {
    infra_pod_spec  = module.query_ydb2_infrastructure_pod.infra-pod-spec
    config_digest   = sha256(data.template_file.empty_configs.rendered)
    query_version   = var.ydb-plugin-version
    watcher_version = var.ydb-plugin-version
  }
}

module "query_ydb2_infrastructure_pod" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_query_prod"
  metrics_agent_conf_path = "${path.module}/files/query-ydb2/metrics-agent.tpl.yaml"
}

resource "ycp_microcosm_instance_group_instance_group" "query_ydb2_ig" {
  name        = "jaeger-query-ydb2-ig-prod"
  description = "Jaeger query installation (YDB)"

  service_account_id = local.ig_sa

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
    environment = local.environment
  }

  platform_l7_load_balancer_spec {
    preferred_ip_version = "IP_VERSION_UNSPECIFIED"
    target_group_spec {
      name = "jaeger-query-ydb2-ig-tg-prod"
      address_names = [
        "ipv6",
      ]
    }
  }

  instance_template {
    service_account_id = local.instance_sa
    platform_id        = "standard-v2"

    resources {
      memory        = 16
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
      conductor-role  = "query"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.query_ydb2_dns_prefix
    }

    metadata = {
      internal-name     = local.query_ydb2_dns_prefix
      internal-hostname = local.query_ydb2_dns_prefix
      internal-fqdn     = local.query_ydb2_dns_prefix
      yandex-dns        = local.query_ydb2_dns_prefix
      docker-config     = data.template_file.docker_json.rendered
      environment       = "prod"
      podmanifest       = data.template_file.query_ydb2_podmanifest.rendered
      infra-configs     = module.query_ydb2_infrastructure_pod.infra-pod-configs
      configs           = data.template_file.empty_configs.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}

resource "ycp_platform_alb_backend_group" "alb_query_ydb2_backend_group" {
  name        = "jaeger-query-ydb2-backend-group"
  description = "Jaeger Query Service Backend Group (YDB)"
  http {
    backend {
      name   = "jaeger-query-ydb-backend"
      weight = 100
      port   = 16686
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.query_ydb2_ig.platform_l7_load_balancer_state[0].target_group_id
      }
      healthchecks {
        timeout             = "0.200s"
        interval            = "1s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        healthcheck_port    = 16688
        http {
          path = "/"
        }
      }
    }
    connection {}
  }
}
