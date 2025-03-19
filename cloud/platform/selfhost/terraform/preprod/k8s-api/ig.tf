locals {
    zones = {
        vla = "ru-central1-a"
        sas = "ru-central1-b"
        myt = "ru-central1-c"
    }

    subnets = {
        vla = "bucpba0hulgrkgpd58qp"
        sas = "bltueujt22oqg5fod2se"
        myt = "fo27jfhs8sfn4u51ak2s"
    }

    secondary_disk_size = 32
    domain = "ycp.cloud-preprod.yandex.net"

    ig_size = 3

    image_id = "fdv8t63uk0ib1tbb5bus"

    name = "mk8s-api-preprod"
    abc_svc = "yckubernetes"
    conductor_group = "k8sapi"
    osquery_tag = "ycloud-svc-k8sapi"

    description = "Managed Kubernetes API"
}

resource ycp_microcosm_instance_group_instance_group group {
    service_account_id = yandex_iam_service_account.ig_sa.id

    name = local.name
    description = local.description

    folder_id = var.yc_folder

    labels = {
        environment = "preprod"
    }

    platform_l7_load_balancer_spec {
        preferred_ip_version = "IP_VERSION_UNSPECIFIED"
        target_group_spec {
            description = format("%s L7 target group", local.description)
            name        = local.name
            address_names = [
              "ipv6",
            ]
        }
    }

    deploy_policy {
        max_creating = 0
        max_deleting = 0
        max_expansion = 0
        max_unavailable = 1
        startup_duration = "10s"
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
            size = local.ig_size
        }
    }

    instance_template {
        service_account_id = "bfb4sriefpc62cd522k7"
        platform_id = "standard-v1"

        labels = {
            abc_svc = local.abc_svc
            env = "pre-prod"
            environment = "preprod"
            layer = "paas"
            conductor-group = local.conductor_group
            yandex-dns = "ig"
        }

        resources {
            memory = 4
            cores = 2
            core_fraction = 20
        }

        boot_disk {
            mode = "READ_WRITE"
            disk_spec {
                type_id = "network-hdd"
                size = 10
                image_id = local.image_id
            }
        }

        secondary_disk {
            device_name = "data"
            mode = "READ_WRITE"
            disk_spec {
                size = local.secondary_disk_size
                type_id = "network-ssd"
            }
        }

        network_interface {
            subnet_ids = values(local.subnets)
            primary_v4_address {
                name = "ipv4"
            }
            primary_v6_address {
                name = "ipv6"
            }
        }

        scheduling_policy {
            termination_grace_period = "10.0s"
        }

        //noinspection HCLSimplifyExpression
        metadata = {
            user-data = file("${path.module}/files/ii/user-data.yaml")
            osquery_tag = local.osquery_tag
            serial-port-enable = "1"
            shortname = format("%s-{instance.internal_dc}-{instance.index_in_zone}", local.name)
            nsdomain = format("%s.%s", "{instance.internal_dc}", local.domain)
            internal-hostname = format("%s-{instance.internal_dc}-{instance.index_in_zone}", local.name)
            internal-name = format("%s-{instance.internal_dc}-{instance.index_in_zone}", local.name)
            
            ssh-keys = "<diff-ignored>"

            k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/bootstrap.yaml")
            skm-config = file("${path.module}/files/ii/skm.yaml")
            base64-decoder-sh = file("${path.module}/files/ii/base64decode_secrets.sh")

            metricsagent-config = file("${path.module}/files/ii/metricsagent.yaml")
            push-client-config = file("${path.module}/files/ii/push-client.yaml")

            yandex-ca = file("${path.module}/../../common/allCAs.pem")
            k8sapi-config = file("${path.module}/files/ii/k8sapi.yaml")
            k8sapi-proxy-config = file("${path.module}/files/ii/k8s-api-proxy.yaml")

            registry-auth = filebase64("${path.module}/files/ii/secrets/docker_config.json.enc")
            metricsagent-auth = filebase64("${path.module}/files/ii/secrets/solomon_oauth_token.enc")

            kubeconfig = filebase64("${path.module}/files/ii/secrets/kubeconfig.enc")
            service-sa-key = filebase64("${path.module}/files/ii/secrets/yc-sa-key.json.enc")
            server-crt = filebase64("${path.module}/files/ii/secrets/server.crt.enc")
            server-key = filebase64("${path.module}/files/ii/secrets/server.key.enc")
        }
    }
}
