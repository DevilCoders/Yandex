locals {
    bill_ipv4_addrs = [
        "172.16.11.88",
        "172.17.11.88",
        "172.18.11.88",
    ]

    bill_ipv6_addrs = [
        "2a02:6b8:c0e:501:0:f806:0:1188",
        "2a02:6b8:c02:901:0:f806:0:1188",
        "2a02:6b8:c03:501:0:f806:0:1188",
    ]

    bill_secondary_disk_size = 16

    bill_image_id = "fdv01iqf4kem851p64jg"

    bill_name = "mk8s-billcollector"
    bill_conductor_group = "mk8s-billcollector"
    bill_osquery_tag = "ycloud-svc-mk8s-billcollector"

    bill_description = "Managed Kubernetes Billing Collector"

    bill_count = 3
}

data "template_file" "mk8s-billcollector-name" {
  count    = local.bill_count
  template = "$${name}"

  vars = {
    name = "${local.bill_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
  }
}

data "template_file" "mk8s-billcollector-fqdn" {
  count    = local.bill_count
  template = "$${name}.$${domain}"

  vars = {
    name = "${local.bill_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
    domain = local.domain
  }
}

resource "yandex_compute_disk" "mk8s-billcollector-disks" {
    count = local.bill_count

    name = "${local.bill_name}-disk-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"

    type = "network-ssd"
    size = local.bill_secondary_disk_size
    zone = element(local.zones, count.index % 3)
}

resource "ycp_compute_instance" "mk8s-billcollector" {
    allow_stopping_for_update = true
    stop_for_metadata_update = true

    count = local.bill_count

    platform_id   = "standard-v2"

    resources {
        cores         = 2
        memory        = 4
        core_fraction = 20
    }

    description = local.bill_description
    service_account_id  = "bfb4sriefpc62cd522k7"

    boot_disk {
        disk_spec {
            type_id = "network-hdd"
            size = 16
            image_id = local.bill_image_id
        }
    }

    zone_id = element(local.zones, count.index % 3)

    network_interface {
        subnet_id    = element(local.subnets, count.index % 3)
        primary_v4_address {
            address = element(local.bill_ipv4_addrs, count.index)
        }
        primary_v6_address {
            address = element(local.bill_ipv6_addrs, count.index)
        }
    }

    secondary_disk {
      auto_delete = false
      disk_id = yandex_compute_disk.mk8s-billcollector-disks[count.index].id
    }

    provisioner "local-exec" {
        when        = destroy
        command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
        environment = {}

        on_failure = continue
    }

    name     = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)
    hostname = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        env = "pre-prod"
        environment = "preprod"
        conductor-group = local.bill_conductor_group
        yandex-dns = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)
        layer = "paas"
    }

    metadata = {
        user-data = file("${path.module}/files/ii/billcollector/user-data.yaml")
        osquery_tag = local.bill_osquery_tag
        serial-port-enable = "1"
        shortname = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)
        internal-name = element(data.template_file.mk8s-billcollector-name.*.rendered, count.index)
        
        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/billcollector/bootstrap.yaml")
        skm-config = file("${path.module}/files/ii/billcollector/skm.yaml")
        base64-decoder-sh = file("${path.module}/files/ii/billcollector/base64decode_secrets.sh")

        metricsagent-config = file("${path.module}/files/ii/billcollector/metricsagent.yaml")
        push-client-config = file("${path.module}/files/ii/billcollector/push-client.yaml")

        billcollector-config = file("${path.module}/files/ii/billcollector/mk8s-billcollector.yaml")

        registry-auth = filebase64("${path.module}/files/ii/billcollector/secrets/docker_config.json.enc")
        metricsagent-auth = filebase64("${path.module}/files/ii/billcollector/secrets/solomon_oauth_token.enc")

        kubeconfig = filebase64("${path.module}/files/ii/billcollector/secrets/kubeconfig.enc")

        test = "test"
    }

    lifecycle {
        ignore_changes = [
            metadata.ssh-keys,
        ]
    }
}
