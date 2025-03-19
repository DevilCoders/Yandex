locals {
    datacenters = [
        "vla",
        "sas",
        "myt",
    ]

    zones = [
        "ru-central1-a",
        "ru-central1-b",
        "ru-central1-c",
    ]

    subnets = [
        "bucpba0hulgrkgpd58qp",
        "bltueujt22oqg5fod2se",
        "fo27jfhs8sfn4u51ak2s",
    ]

    ipv4_addrs = [
        "172.16.0.42",
        "172.17.0.23",
        "172.18.0.34",
    ]

    ipv6_addrs = [
        "2a02:6b8:c0e:501:0:f806:0:201",
        "2a02:6b8:c02:901:0:f806:0:201",
        "2a02:6b8:c03:501:0:f806:0:201",
    ]

    domain = "ycp.cloud-preprod.yandex.net"

    environment = "preprod"

    abc_svc = "yckubernetes"

    secondary_disk_size = 128

    image_id = "fdv7086egic8j0ooabmq"

    name = "k8s-etcd-preprod"
    conductor_group = "mk8s-etcd"
    osquery_tag = "ycloud-svc-mk8s-etcd"

    description = "Managed Kubernetes Backend Etcd"

    count = 3
}

data "template_file" "mk8s-etcd-name" {
  count    = local.count
  template = "$${name}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "node-disk-name" {
  count = local.count
  template = "$${name}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}-disk"
  }
}

resource "yandex_compute_disk" "node-data-disk" {
  count = local.count

  name = data.template_file.node-disk-name[count.index].rendered

  zone = element(local.zones, count.index)

  size = local.secondary_disk_size
  type = "network-ssd"
}

data "template_file" "user-data" {
  count = local.count
  template = file("${path.module}/files/ii/user-data.tpl.yaml")

  vars = {
    disk_id = yandex_compute_disk.node-data-disk[count.index].id
  }
}

data "template_file" "etcd-pod" {
  count    = local.count
  template = "${path.module}/files/ii/etcd-$${dc_idx}.yaml"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "etcd-events-pod" {
  count    = local.count
  template = "${path.module}/files/ii/etcd-events-$${dc_idx}.yaml"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "healthcheck-client-crt" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/healthcheck-client-$${dc_idx}.crt.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "healthcheck-client-key" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/healthcheck-client-$${dc_idx}.key.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "peer-crt" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/peer-$${dc_idx}.crt.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "peer-key" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/peer-$${dc_idx}.key.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "server-crt" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/server-$${dc_idx}.crt.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

data "template_file" "server-key" {
  count    = local.count
  template = "${path.module}/files/ii/secrets/server-$${dc_idx}.key.enc"

  vars = {
    dc_idx = "${element(local.datacenters, count.index % 3)}0${floor(count.index / 3) + 1}"
  }
}

resource "ycp_compute_instance" "mk8s-etcd" {
    allow_stopping_for_update = true
    stop_for_metadata_update = true

    count = local.count

    platform_id   = "standard-v1"

    resources {
        cores         = 4
        memory        = 16
        core_fraction = 100
    }

    description = local.description
    service_account_id  = "bfbf1pqt4blom54ii42u"

    boot_disk {
        disk_spec {
            type_id = "network-hdd"
            size = 80
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
        disk_id = yandex_compute_disk.node-data-disk[count.index].id
    }

    provisioner "local-exec" {
        when        = destroy
        command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
        environment = {}

        on_failure = continue
    }

    name = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
    hostname = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        cluster_id = "vrtwuryrdpysh3gm"
        conductor-dc         = element(local.datacenters, count.index % 3)
        conductor-group = local.conductor_group
        env                  = "pre-prod"
        environment = "preprod"
        layer = "paas"
        node_id              = "${count.index}"
        osquery_tag = local.osquery_tag
        role                 = "etcd"
        skip_update_ssh_keys = "false"
        yandex-dns           = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
    }

    metadata = {
        ssh-keys = module.ssh-keys.ssh-keys
        yandex-dns = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
        user-data = element(data.template_file.user-data.*.rendered, count.index)
        osquery_tag = local.osquery_tag
        serial-port-enable = "1"
        shortname = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
        internal-name = element(data.template_file.mk8s-etcd-name.*.rendered, count.index)
        
        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/bootstrap.yaml")
        skm-config = file("${path.module}/files/ii/skm.yaml")
        base64-decoder-sh = file("${path.module}/files/ii/base64decode_secrets.sh")

        metricsagent-config = file("${path.module}/files/ii/metricsagent.yaml")
        etcd-pod = file(element(data.template_file.etcd-pod.*.rendered, count.index))
        etcd-events-pod = file(element(data.template_file.etcd-events-pod.*.rendered, count.index))

        registry-auth = filebase64("${path.module}/files/ii/secrets/docker_config.json.enc")
        metricsagent-auth = filebase64("${path.module}/files/ii/secrets/solomon_oauth_token.enc")

        ca-crt = filebase64("${path.module}/files/ii/secrets/etcd-ca.crt.enc")

        healthcheck-client-crt = filebase64(element(data.template_file.healthcheck-client-crt.*.rendered, count.index))
        healthcheck-client-key = filebase64(element(data.template_file.healthcheck-client-key.*.rendered, count.index))
        peer-crt = filebase64(element(data.template_file.peer-crt.*.rendered, count.index))
        peer-key = filebase64(element(data.template_file.peer-key.*.rendered, count.index))
        server-crt = filebase64(element(data.template_file.server-crt.*.rendered, count.index))
        server-key = filebase64(element(data.template_file.server-key.*.rendered, count.index))
    }

    lifecycle {
        ignore_changes = [
            metadata.ssh-keys,
        ]
    }
}
