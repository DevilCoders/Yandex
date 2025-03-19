resource "ycp_microcosm_instance_group_instance_group" "yc_search_marketplace_backend" {
#  lifecycle {
#    prevent_destroy = true
#  }

  name               = var.marketplace_backend_prefix
  folder_id          = var.yc_folder
  service_account_id = var.deploy_service_account_id

  scale_policy {
    fixed_scale {
      size = 2
    }
  }

  allocation_policy {
    dynamic "zone" {
      for_each = var.yc_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_creating     = 0
    # Limit the number of instance being destroyed at once so that the devops which accidentally destroyed the instances could have some time to react.
    max_deleting     = 1
    # DO NOT change, otherwise the instances will be deploy in blue-green style (creating the new instance and then destroying the old one)
    max_expansion    = 0
    max_unavailable  = var.max_unavailable
    startup_duration = var.startup_duration # Some timeout just to be sure everything is up =)
  }

  labels = {
    abc_svc = "yc-search"
    env     = "il"
    skip_update_ssh_keys = true
  }

  health_checks_spec {
    health_check_spec {
      http_options {
        port = var.healthcheck_port
        path = var.healthcheck_path
      }
      interval            = var.ig_healthcheck_interval
      timeout             = var.ig_healthcheck_timeout
      healthy_threshold   = var.ig_healthcheck_healthy
      unhealthy_threshold = var.ig_healthcheck_unhealthy
    }
    max_checking_health_duration = var.max_checking_health_duration
  }

  dynamic "variable" {
    for_each = var.ipv6_addresses
    content {
      key = "instance-${variable.key + 1}-ipv6-address"
      value = variable.value
    }
  }

  dynamic "variable" {
    for_each = var.ipv4_addresses
    content {
      key = "instance-${variable.key + 1}-ipv4-address"
      value = variable.value
    }
  }

  instance_template {
    name     = "${var.marketplace_backend_prefix}-{instance.internal_dc}-{instance.index_in_zone}"
    hostname = "${var.marketplace_backend_prefix}-{instance.internal_dc}-{instance.index_in_zone}"
    fqdn     = "${var.marketplace_backend_prefix}-{instance.internal_dc}-{instance.index_in_zone}.${var.hostname_suffix}"

    service_account_id = var.service_account_id

    platform_id = var.instance_platform_id

    resources {
      cores         = 4
      memory        = 8
      core_fraction = 100
    }

    boot_disk {
      mode = "READ_WRITE"
      disk_spec {
        type_id  = var.instance_disk_type
        size     = var.instance_disk_size
        image_id = var.marketplace_backend_image_id
      }
    }

    secondary_disk {
      device_name = "data"
      disk_spec {
        size    = 10
        type_id = "network-ssd"
      }
    }

    secondary_disk {
      device_name = "logs"
      disk_spec {
        size    = 30
        type_id = "network-hdd"
      }
    }

    network_interface {
      subnet_ids = [
      for zone in var.yc_zones :
      var.subnets[zone]
      ]
      primary_v4_address {
        address = "{instance-{instance.index}-ipv4-address}"
        name    = "ipv4"
      }
      primary_v6_address {
        address = "{instance-{instance.index}-ipv6-address}"
        name    = "ipv6"
        dns_record_spec {
          fqdn        = "${var.marketplace_backend_prefix}-{instance.internal_dc}-{instance.index_in_zone}"
          dns_zone_id = var.dns_zone_id
          ptr         = true
          ttl         = 300
        }
      }
      security_group_ids = var.security_group_ids
    }

    placement_policy {
#      host_group = var.host_group
      placement_group_id = var.yc-search-backend-pg
    }

    metadata = {
      skm = file("${path.module}/files/skm/skm-bundle.yaml")
      k8s-runtime-bootstrap-yaml = file("${path.module}/../common/bootstrap.yaml")
#      application-config = file("${path.module}/files/trail/application.tpl.yaml")
#      solomonagent-config = file("${path.module}/../common/solomon-agent.conf")
#      metricsagent-config = file("${path.module}/../common/metricsagent.yaml")
#      push-client-config = file("${path.module}/files/push-client/push-client.yaml")
#      jaeger-agent-config = file("${path.module}/../common/jaeger-agent.yaml")
#      solomon-agent-pod = templatefile("${path.module}/../common/solomon-agent-pod.tpl.yaml", {
#        solomon_version = var.solomon_agent_image_version
#      })
      dnsmasq-nameservers = file("${path.module}/files/resolv.dnsmasq")
      application-pod = templatefile("${path.module}/files/manifests/marketplace_backend/podmanifest.tpl.yaml", {
        application_version    = var.marketplace_backend_docker_version
        cloud_zone    = "{instance.internal_dc}"
        cloud_index = "{instance.index_in_zone}"
        prefix = var.marketplace_backend_prefix
      })
      logrotate-pod = templatefile("${path.module}/files/manifests/logrotate/logrotate_manifest.tpl.yaml", {
        logrotate_version    = var.marketplace_backend_logrotate_docker_version
        prefix = var.backend_prefix
      })
      solomon-agent-pod = templatefile("${path.module}/files/manifests/solomon-agent/solomon-agent-pod.tpl.yaml", {
        solomon_version    = var.solomon_version
      })
      solomonagent-config = file("${path.module}/../common/solomon-agent.conf")
      nsdomain = var.hostname_suffix
      user-data = file("${path.module}/../common/cloud-init.yaml")
      enable-oslogin = true
#      osquery_tag = "ycloud-svc-cloud-trail"
    }
  }
}

