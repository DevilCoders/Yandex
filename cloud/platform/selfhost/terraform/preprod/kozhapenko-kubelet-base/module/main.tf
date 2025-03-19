locals {
    zones = {
        myt = "ru-central1-c"
    }

    domain = var.installation == "prod" ? "ycp.cloud.yandex.net" : "ycp.cloud-preprod.yandex.net"
    ssh-keys-path = var.ssh_keys_path != "" ? var.ssh_keys_path : "${path.module}/files/ssh-keys"
    is-prod = var.installation == "prod" ? true : false
}

provider "ycp" {
    prod = local.is-prod
}

data "template_cloudinit_config" "userdata" {
    gzip = false
    base64_encode = false
    part {
        content_type = "text/cloud-config"
        content = file("${path.module}/files/user-data-parts/cloud-config.yaml")
    }
}

resource ycp_microcosm_instance_group_instance_group group {
    service_account_id = var.sa_ig

    name = "kozhapenko-paas-base-g2"
    description = "test ig for paas-base-g2 image"

    folder_id = var.folder_id

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
            abc_svc = "cloud-platform-deploy"
            env = var.installation
            environment = var.installation
            conductor-group = "kozhapenko-paas-base-g2"
            yandex-dns = "ig"
        }

        resources {
            memory = var.instance_memory
            cores = 2
            core_fraction = 50
        }

        boot_disk {
            mode = "READ_WRITE"
            disk_spec {
                type_id = "network-hdd"
                size = var.disk_size
                image_id = var.image_id
            }
        }

        secondary_disk {
            device_name = "data"
            mode = "READ_WRITE"
            disk_spec {
                size = var.sec_disk_size
                type_id = "network-hdd"
            }
        }

        network_interface {
            subnet_ids = values(var.subnets)
            primary_v4_address {}
            primary_v6_address {}
        }

        scheduling_policy {
            termination_grace_period = "30.0s"
        }

        //noinspection HCLSimplifyExpression
        metadata = {
            serial-port-enable = "1"
            user-data = data.template_cloudinit_config.userdata.rendered
            skm-config-yaml = " "
            k8s-runtime-bootstrap-yaml = file("${path.module}/files/metadata-attributes/k8s-runtime-bootstrap.yaml")
            jaeger-config-yaml = file("${path.module}/files/metadata-attributes/jaeger-config.yaml")
            ssh-keys = file(local.ssh-keys-path)
            shortname = "kozhapenko-paas-base-g2-{instance.index}"
            nsdomain = local.domain
            redeploy = var.redeploy
        }
    }

    lifecycle {
        ignore_changes = [
            instance_template.0.metadata["ssh-keys"]
        ]
    }
}
