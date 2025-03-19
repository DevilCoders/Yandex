resource "ycp_microcosm_instance_group_instance_group" "vpc_test_billing" {
  folder_id = var.folder_id

  name = "billing-test-ig"
  description = "Group of test accounting services, used for filling test billing topic"
  service_account_id = ycp_iam_service_account.yc_vpc_accounting_ig_sa.id
  labels = {}

  instance_template {
    name = "vpc-billing-test-{instance.index_in_zone}-{instance.internal_dc}"

    description = "Just one of billing-test accounting backends"
    hostname = "{instance.index_in_zone}.{instance.internal_dc}.billing-test.${var.dns_zone}"
    service_account_id = ycp_iam_service_account.yc_vpc_billing_sa.id
    
    boot_disk {
      disk_spec {
        image_id = var.accounting_image
        size = 15
      }
    }

    platform_id = var.vm_platform_id
    resources {
      memory = 16
      cores = 16
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for s in ycp_vpc_subnet.subnets : s.id]
      security_group_ids = [yandex_vpc_security_group.accounting.id]
      
      primary_v4_address {
        name = "ipv4"
      }
      primary_v6_address {
        name = "ipv6"
        dns_record_spec {
          fqdn = "{instance.index_in_zone}.{instance.internal_dc}.billing-test.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
      }
    }

    metadata = {
      user-data = templatefile("${path.module}/files/cloud-init.yaml",
        {
          hostname = "{instance.index_in_zone}.{instance.internal_dc}.billing-test.${var.dns_zone}",
          files = [
              {
                path = "/etc/yc/vpc-accounting/accounting.env"
                content = templatefile("${path.module}/files/vpc-accounting/accounting.env", {
                  accounting_cmd = "yc-vpc-accounting"
                })
              },
              {
                path = "/etc/yc/vpc-accounting/config.yaml",
                content = templatefile("${path.module}/files/vpc-accounting/config.yaml", {
                  mode = "accounting"
                  log_level = var.log_level
                  environment = var.ycp_profile
                  cache_url = "cache.${var.dns_zone}:443"
                  endpoint = var.logbroker_endpoint
                  input_topic = var.logbroker_accounting_topic
                  input_count = 16
                  billing_database = var.logbroker_billing_database
                  accounting_database = var.logbroker_accounting_database
                  output_topic = var.logbroker_billing_test_topic
                  consumer = var.logbroker_billing_test_consumer
                  loadbalancer_topic = var.logbroker_loadbalancer_test_topic
                  aggregate_duration = "1m"
                  pull_interval = var.solomon_pull_interval
                  unchanged_metrics_lifetime = var.unchanged_metrics_lifetime
                  marshaller_goroutines = 6
                  marshaller_batch_bytes = 10485760
                  marshaller_batch_duration = "5s"
                  marshals_per_goroutine = 400
                  bill_elb_egress_traffic = true
                  bill_healtcheck_traffic = true
                  bill_unknown_balancers = true
                  bill_internal_balancers = true
                  compression_level = 9
                })
              },
              {
                path = "/etc/yandex/unified_agent/conf.d/balancer-metrics.yml",
                content = templatefile("${path.module}/files/vpc-accounting/balancer-metrics.yml", {
                  solomon_url = var.solomon_url
                  output_channel = "metrics_pull_channel"
                })
              },
              {
                path = "/etc/yandex/unified_agent/conf.d/logbroker-output.yml",
                content = templatefile("${path.module}/files/vpc-accounting/logbroker-output.yml", {
                  logbroker_endpoint = var.logbroker_endpoint
                  logbroker_database = var.logbroker_billing_database
                })
              },
              {
                path = "/etc/.stand",
                content = var.ycp_profile
              }
          ]
        }
      )
      enable-oslogin = "true"
    }

    labels = {
      layer = "iaas"
      abc_svc = "ycvpc"
      env = var.environment
    }
  }

  allocation_policy {
    dynamic zone {
      for_each = var.accounting_network_subnet_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 0
    max_creating = 6
    max_expansion = 3
    max_deleting = 6
  }

  scale_policy {
    fixed_scale {
      size = var.test_instances_count
    }
  }

  health_checks_spec {
    health_check_spec {
      address_names = ["ipv6"]
      interval = "20s"
      timeout = "5s"
      healthy_threshold = 2
      unhealthy_threshold = 9
      http_options {
        port = 80
        path = "/"
      }
    }
  }
}
