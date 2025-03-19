data "template_file" "query_podmanifest2" {
  template = file("${path.module}/files/query2/pod.tpl.yaml")
  vars = {
    infra_pod_spec  = module.query_infrastructure_pod2.infra-pod-spec
    config_digest   = sha256(data.template_file.configs.rendered)
    query_version   = var.ydb-plugin-version
    watcher_version = var.ydb-plugin-version
  }
}

module "query_infrastructure_pod2" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_query_preprod"
  metrics_agent_conf_path = "${path.module}/files/query2/metrics-agent.tpl.yaml"
}

resource "ycp_microcosm_instance_group_instance_group" "query_ig2" {
  name        = "jaeger-query-ig2-preprod"
  description = "Jaeger query installation (L7)"

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
      name = "jaeger-query-ig2-tg-preprod"
      address_names = [
        "ipv6",
      ]
    }
  }

  instance_template {
    service_account_id = "bfblesupq182q80mpq26"
    platform_id        = "standard-v2"

    resources {
      memory        = 6
      cores         = 2
      core_fraction = 20
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
      conductor-role  = "query"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.query_dns_prefix2
    }

    metadata = {
      internal-name     = local.query_dns_prefix2
      internal-hostname = local.query_dns_prefix2
      internal-fqdn     = local.query_dns_prefix2
      yandex-dns        = local.query_dns_prefix2
      docker-config     = data.template_file.docker_json.rendered
      environment       = "preprod"
      podmanifest       = data.template_file.query_podmanifest2.rendered
      infra-configs     = module.query_infrastructure_pod2.infra-pod-configs
      configs           = data.template_file.configs.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}

resource "ycp_platform_alb_backend_group" "alb_query_backend_group2" {
  name        = "jaeger-query2-backend-group"
  description = "Jaeger Query Service Backend Group"
  http {
    backend {
      name   = "jaeger-query-backend"
      weight = 100
      port   = 16686
      target_group {
        target_group_id = ycp_microcosm_instance_group_instance_group.query_ig2.platform_l7_load_balancer_state[0].target_group_id
      }
      healthchecks {
        timeout             = "0.200s"
        interval            = "1s"
        healthy_threshold   = 2
        unhealthy_threshold = 3
        healthcheck_port    = 15001
        http {
          path = "/ping"
        }
      }
    }
    connection {}
  }
}
