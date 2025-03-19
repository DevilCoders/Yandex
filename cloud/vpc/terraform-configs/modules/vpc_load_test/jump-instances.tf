resource "ycp_microcosm_instance_group_instance_group" "vpc_load_test" {
  folder_id = var.folder_id

  name = "vpc-load-test-ig"
  description = "Jump host IG used to access k8s cluster"
  service_account_id = ycp_iam_service_account.yc_vpc_load_test_ig_sa.id
  labels = {}

  instance_template {
    name = "jumphost-{instance.index_in_zone}-{instance.internal_dc}"

    description = "Just one of load_test load_test backends"
    service_account_id = ycp_iam_service_account.yc_vpc_load_test_ig_sa.id
    
    boot_disk {
      disk_spec {
        image_id = var.jump_image
        size = 15
      }
    }

    platform_id = "standard-v2"
    resources {
      memory = 4
      cores = 4
    }

    network_interface {
      network_id = ycp_vpc_network.network.id
      subnet_ids = [for s in ycp_vpc_subnet.subnets : s.id]
      security_group_ids = [yandex_vpc_security_group.load_test.id]
      
      primary_v4_address {
        name = "ipv4"
      }
      primary_v6_address {
        name = "ipv6"
        dns_record_spec {
          fqdn = "{instance.index_in_zone}.{instance.internal_dc}.jump.${var.dns_zone}."
          dns_zone_id = var.dns_zone_id
          ptr = true
        }
        dns_record_spec {
          fqdn = "jump.${var.dns_zone}." 
          dns_zone_id = var.dns_zone_id
        }
      }
    }

    metadata = {
      user-data = templatefile("${path.module}/files/cloud-init.yaml",
        {
          hostname = "{instance.index_in_zone}.{instance.internal_dc}.internal",
          files = [
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
      for_each = var.load_test_network_subnet_zones
      content {
        zone_id = zone.value
      }
    }
  }

  deploy_policy {
    max_unavailable = 0
    max_creating = 3
    max_expansion = 1
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = 1
    }
  }
}
