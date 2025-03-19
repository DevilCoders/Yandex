locals {
    ipv4_addrs = [
        "172.16.1.30",
    ]

    ipv6_addrs = [
        "2a02:6b8:c0e:2c0:0:fc1a:0:30",
    ]

    secondary_disk_size = 32

    # l7-xds-2020-06-11t15-48-29z
    image_id = "c2ph5gl81t6hsn3lke1q"

    name = "xds-testing"

    conductor_group = "xds"
    osquery_tag = "ycloud-svc-xds"

    description = "XDS service"
    instances_count = 1
}

data "template_file" "xds-name" {
  count    = local.instances_count
  template = "$${name}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
  }
}

data "template_file" "xds-fqdn" {
  count    = local.instances_count
  template = "$${name}.$${domain}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
    domain = local.domain
  }
}

resource "ycp_compute_disk" "xds-disks" {
    count = local.instances_count

    name = "${local.name}-disk-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"

    type_id = "network-ssd"
    size = local.secondary_disk_size
    zone_id = element(local.zones, count.index % 3)
}

data "template_file" "xds-config" {
    count = local.instances_count
    template = file("${path.module}/files/xds.tmpl.yaml")

    vars = {
        id     = element(data.template_file.xds-name.*.rendered, count.index)
        region = local.zone_regions[element(local.zones, count.index)]
        zone   = element(local.zones, count.index)
    }
}

resource "ycp_compute_instance" "xds" {
    allow_stopping_for_update = true
    stop_for_metadata_update = true
    count = local.instances_count

    platform_id   = "standard-v2"

    resources {
        cores         = 2
        memory        = 2
        core_fraction = 20
    }

    description = local.description
    service_account_id  = ycp_iam_service_account.xds_alb_private_api_sa.id

    boot_disk {
        disk_spec {
            type_id = "network-hdd"
            size = 32
            image_id = local.image_id
        }
    }

    zone_id = element(local.zones, count.index % 3)

    network_interface {
        subnet_id    = element(local.subnets, count.index % 3)
        primary_v4_address {
            address = element(local.ipv4_addrs, count.index)
        }
        primary_v6_address {
            address = element(local.ipv6_addrs, count.index)
        }
    }

    secondary_disk {
      auto_delete = false
      disk_id = ycp_compute_disk.xds-disks[count.index].id
    }

    # provisioner "local-exec" {
    #     when        = destroy
    #     command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
    #     environment = {}

    #     on_failure = continue
    # }

    name     = element(data.template_file.xds-name.*.rendered, count.index)
    hostname = element(data.template_file.xds-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        env = local.environment
        environment = local.environment
        conductor-group = local.conductor_group
        yandex-dns = element(data.template_file.xds-name.*.rendered, count.index)
        layer = "paas"
    }

    metadata = {
        user-data = file("${path.module}/files/user_data.yaml")
        osquery_tag = local.osquery_tag
        serial-port-enable = "1"
        kms-endpoint = "kms.cloud-testing.yandex.net"
        shortname = element(data.template_file.xds-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.xds-name.*.rendered, count.index)
        internal-name = element(data.template_file.xds-name.*.rendered, count.index)

        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/bootstrap.yaml")
        metricsagent-config = file("${path.module}/files/metrics-agent.yaml")
        platform-http-checks = file("${path.module}/files/platform-http-checks.json")

        xds-config = element(data.template_file.xds-config.*.rendered, count.index)
        yandex-ca = file("${path.module}/files/allCAs.pem")

        xds_server_cert_crt = file("${path.module}/files/secrets/xds-server.pem")
        xds_server_cert_key_kms = file("${path.module}/files/secrets/xds-server_key.json")

        xds_config_dump_access_key = file("${path.module}/files/secrets/xds-config-dump-access-key.txt")
        xds_config_dump_secret_key_kms = file("${path.module}/files/secrets/xds-config-dump-secret-key.json")

        xds_routes_access_key = file("${path.module}/files/secrets/xds-routes-access-key.txt")
        xds_routes_secret_key_kms = file("${path.module}/files/secrets/xds-routes-secret-key.json")

        juggler-manifest-json = file("${path.module}/files/MANIFEST.json")

        # metricsagent-auth = filebase64("${path.module}/files/secrets/solomon_oauth_token.enc")
    }
}
