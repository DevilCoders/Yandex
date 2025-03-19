data "template_file" "reader_global_podmanifest2" {
  template = file("${path.module}/files/reader_global2/pod.tpl.yaml")
  vars = {
    infra_pod_spec    = module.reader_global_infrastructure_pod2.infra-pod-spec
    config_digest     = sha256(data.template_file.reader_global_configs2.rendered)
    reader_version    = var.lb-reader-version
    collector_version = var.ydb-plugin-version
  }
}

module "reader_global_infrastructure_pod2" {
  source                  = "../../modules/infrastructure-pod-metrics-agent"
  juggler_check_name      = "envoy-ping"
  solomon_shard_cluster   = "jaeger_collector_preprod"
  metrics_agent_conf_path = "${path.module}/files/reader_global2/metrics-agent.tpl.yaml"
}

data "template_file" "reader_global_config2" {
  template = file("${path.module}/files/reader_global2/config.tpl.yaml")
}

data "template_file" "reader_global_configs2" {
  template = file("${path.module}/files/reader_global2/configs.tpl.json")
  vars = {
    jaeger_lb_reader_config = data.template_file.reader_global_config2.rendered
    jaeger_lb_reader_token  = module.jaeger-lb-token.value
  }
}

resource "ycp_microcosm_instance_group_instance_group" "reader_global_ig2" {
  name        = "jaeger-lb-reader-global-ig2-preprod"
  description = "Jaeger LB reader global cluster"

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

  instance_template {
    service_account_id = "bfblesupq182q80mpq26"
    platform_id        = "standard-v2"

    resources {
      memory        = 4
      cores         = 4
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
      conductor-role  = "lb_reader"
      conductor-dc    = "{instance.internal_dc}"
      yandex-dns      = local.reader_global_dns_prefix2
    }

    metadata = {
      internal-name     = local.reader_global_dns_prefix2
      internal-hostname = local.reader_global_dns_prefix2
      internal-fqdn     = local.reader_global_dns_prefix2
      yandex-dns        = local.reader_global_dns_prefix2
      docker-config     = data.template_file.docker_json.rendered
      environment       = "preprod"
      podmanifest       = data.template_file.reader_global_podmanifest2.rendered
      infra-configs     = module.reader_global_infrastructure_pod2.infra-pod-configs
      configs           = data.template_file.reader_global_configs2.rendered
      ssh-keys          = module.ssh-keys.ssh-keys
      // TODO: enable ssh key updated after abc svc creation
      skip_update_ssh_keys = true
    }
  }
}
