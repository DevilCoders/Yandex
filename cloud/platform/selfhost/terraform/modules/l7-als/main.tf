locals {
    zones = {
        vla = "ru-central1-a"
        sas = "ru-central1-b"
        myt = "ru-central1-c"
    }

    user-data-path = var.user_data_path != "" ? var.user_data_path : "${path.module}/config/user-data.yaml"
    secondary-disk-size = var.installation == "prod" ? 300 : 100
    domain = var.installation == "prod" ? "ycp.cloud.yandex.net" : "ycp.cloud-preprod.yandex.net"
    is-prod = var.installation == "prod" ? true : false
}

provider "ycp" {
    prod = local.is-prod
}

resource ycp_microcosm_instance_group_instance_group group {
    service_account_id = var.sa_ig

    name = "l7-als"
    description = "IG-managed ALS cluster"

    folder_id = var.folder_id

    load_balancer_spec {
        target_group_spec {
            name = "als-tg"
            labels        = {
                layer   = "paas"
                abc_svc = "ycl7"
            }
            address_names = [
                "ipv6",
            ]
        }
    }

    health_checks_spec {
        health_check_spec {
            http_options {
                port = "80"
                path = "/health"
            }
            interval = "5s"
            timeout = "2s"
            unhealthy_threshold = 2
            healthy_threshold = 4
        }
    }

    deploy_policy {
        max_creating = 0
        max_deleting = 0
        max_expansion = 0
        max_unavailable = 1
        startup_duration = var.startup_duration
    }

    allocation_policy {
        dynamic zone {
            for_each = values(local.zones)
            content {
                zone_id = zone.value
            }
        }
    }

    scale_policy {
        fixed_scale {
            size = var.ig_size
        }
    }

    instance_template {
        service_account_id = var.sa_kms
        platform_id = "standard-v2"

        labels = {
            abc_svc = "ycl7"
            env = var.installation
            environment = var.installation
            conductor-group = "l7-api-als"
            yandex-dns = "ig"
        }

        resources {
            memory = 4
            cores = 2
            core_fraction = var.core_fraction
        }

        boot_disk {
            mode = "READ_WRITE"
            disk_spec {
                type_id = "network-hdd"
                size = 10
                image_id = var.image_id
            }
        }

        secondary_disk {
            device_name = "data"
            mode = "READ_WRITE"
            disk_spec {
                size = local.secondary-disk-size
                type_id = "network-ssd"
            }
        }

        network_interface {
            subnet_ids = values(var.subnets)
            primary_v4_address {
                name = "ipv4"
            }
            primary_v6_address {
                name = "ipv6"
            }
        }

        scheduling_policy {
            termination_grace_period = "600.0s"
        }

        //noinspection HCLSimplifyExpression
        metadata = {
            user-data = file(local.user-data-path)
            osquery_tag = "ycloud-svc-ycl7"
            serial-port-enable = "1"
            ssh-keys = "<diff-ignored>"
            shortname = "l7-als-{instance.internal_dc}-{instance.index_in_zone}"
            nsdomain = format("%s.%s", "{instance.internal_dc}", local.domain)
            redeploy = var.redeploy
        }
    }
}
