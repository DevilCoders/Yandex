data "template_file" "reader_ydb2_podmanifest" {
  template = file("${path.module}/files/reader-ydb2/pod.tpl.yaml")
  vars = {
    infra_pod_spec    = module.reader_ydb2_infrastructure_pod.infra-pod-spec
    config_digest     = sha256(data.template_file.reader_ydb2_configs.rendered)
    reader_version    = var.lb-reader-version
    collector_version = var.ydb-plugin-version
  }
}

module "reader_ydb2_infrastructure_pod" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_collector_prod"
  metrics_agent_conf_path = "${path.module}/files/reader-ydb2/metrics-agent.tpl.yaml"
}

data "template_file" "reader_ydb2_config" {
  template = file("${path.module}/files/reader-ydb2/config.tpl.yaml")
}

data "template_file" "reader_ydb2_configs" {
  template = file("${path.module}/files/reader-ydb2/configs.tpl.json")
  vars = {
    jaeger_lb_reader_config = data.template_file.reader_ydb2_config.rendered
    jaeger_lb_reader_token  = module.jaeger-lb-token.value
  }
}

resource "ycp_microcosm_instance_group_instance_group" "reader_ydb2_ig" {
  name        = "jaeger-lb-reader-ydb2-ig-prod"
  description = "Jaeger LB reader cluster (YDB)"

  service_account_id = local.ig_sa

  scale_policy {
    fixed_scale {
      size = 6
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
    environment = "prod"
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
      conductor-role  = "lb_reader"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.reader_ydb2_dns_prefix
    }

    metadata = {
      internal-name     = local.reader_ydb2_dns_prefix
      internal-hostname = local.reader_ydb2_dns_prefix
      internal-fqdn     = local.reader_ydb2_dns_prefix
      yandex-dns        = local.reader_ydb2_dns_prefix
      docker-config     = data.template_file.docker_json.rendered
      environment       = "prod"
      podmanifest       = data.template_file.reader_ydb2_podmanifest.rendered
      infra-configs     = module.reader_ydb2_infrastructure_pod.infra-pod-configs
      configs           = data.template_file.reader_ydb2_configs.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}
