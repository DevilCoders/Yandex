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
        "e9b9e47n23i7a9a6iha7",
        "e2lt4ehf8hf49v67ubot",
        "b0c7crr1buiddqjmuhn7",
    ]

    ipv4_addrs = [
        "172.16.15.88",
        "172.17.15.88",
        "172.18.15.88",
    ]

    ipv6_addrs = [
        "2a02:6b8:c0e:500:0:f803:0:1588",
        "2a02:6b8:c02:900:0:f803:0:1588",
        "2a02:6b8:c03:500:0:f803:0:1588",
    ]

    domain = "ycp.cloud.yandex.net"

    environment = "prod"

    abc_svc = "yckubernetes"

    secondary_disk_size = 64

    image_id = "fd83tp7nfmcnnas9nj1k"

    name = "mk8s-master-prod"
    conductor_group = "mk8s-master"
    osquery_tag = "ycloud-svc-mk8s-master"

    description = "Managed Kubernetes Backend API Server"

    count = 3
}

data "template_file" "mk8s-master-name" {
  count    = local.count
  template = "$${name}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
  }
}

data "template_file" "mk8s-master-fqdn" {
  count    = local.count
  template = "$${name}.$${domain}"

  vars = {
    name = "${local.name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
    domain = local.domain
  }
}

resource "yandex_compute_disk" "mk8s-master-disks" {
    count = local.count

    name = "${local.name}-disk-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"

    type = "network-ssd"
    size = local.secondary_disk_size
    zone = element(local.zones, count.index % 3)
}

resource "ycp_compute_instance" "mk8s-master" {
    allow_stopping_for_update = true
    stop_for_metadata_update = true

    count = local.count

    platform_id   = "standard-v2"

    resources {
        cores         = 2
        memory        = 16
        core_fraction = 100
    }

    description = local.description
    service_account_id  = "ajeku6995q19giogjavr"

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
      disk_id = yandex_compute_disk.mk8s-master-disks[count.index].id
    }

    provisioner "local-exec" {
        when        = destroy
        command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
        environment = {}

        on_failure = continue
    }

    name     = element(data.template_file.mk8s-master-name.*.rendered, count.index)
    hostname = element(data.template_file.mk8s-master-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        env = "prod"
        environment = "prod"
        conductor-group = local.conductor_group
        yandex-dns = element(data.template_file.mk8s-master-name.*.rendered, count.index)
        layer = "paas"
    }

    metadata = {
        user-data = file("${path.module}/files/ii/user-data.yaml")
        osquery_tag = local.osquery_tag
        serial-port-enable = "1"
        shortname = element(data.template_file.mk8s-master-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.mk8s-master-name.*.rendered, count.index)
        internal-name = element(data.template_file.mk8s-master-name.*.rendered, count.index)
        
        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/bootstrap.yaml")
        skm-config = file("${path.module}/files/ii/skm.yaml")
        base64-decoder-sh = file("${path.module}/files/ii/base64decode_secrets.sh")

        metricsagent-config = file("${path.module}/files/ii/metricsagent.yaml")
        kube-apiserver-pod = file("${path.module}/files/ii/kube-apiserver-${element(local.datacenters, count.index % 3)}.yaml")

        registry-auth = filebase64("${path.module}/files/ii/secrets/docker_config.json.enc")
        metricsagent-auth = filebase64("${path.module}/files/ii/secrets/solomon_oauth_token.enc")

        admin-kubeconfig = filebase64("${path.module}/files/ii/secrets/admin-kubeconfig.conf.enc")
        kube-controller-manager-kubeconfig = filebase64("${path.module}/files/ii/secrets/kube-controller-manager-kubeconfig.conf.enc")
        kube-scheduler-kubeconfig = filebase64("${path.module}/files/ii/secrets/scheduler-kubeconfig.conf.enc")
        metricsagent-kubeconfig = filebase64("${path.module}/files/ii/secrets/admin-kubeconfig.conf.enc")

        apiserver-crt = filebase64("${path.module}/files/ii/secrets/apiserver.crt.enc")
        apiserver-key = filebase64("${path.module}/files/ii/secrets/apiserver.key.enc")
        ca-crt = filebase64("${path.module}/files/ii/secrets/ca.crt.enc")
        ca-key = filebase64("${path.module}/files/ii/secrets/ca.key.enc")
        etcd-apiserver-client-crt = filebase64("${path.module}/files/ii/secrets/etcd-apiserver-client.crt.enc")
        etcd-apiserver-client-key = filebase64("${path.module}/files/ii/secrets/etcd-apiserver-client.key.enc")
        etcd-ca-crt = filebase64("${path.module}/files/ii/secrets/etcd-ca.crt.enc")
        etcd-ca-key = filebase64("${path.module}/files/ii/secrets/etcd-ca.key.enc")
        front-proxy-ca-crt = filebase64("${path.module}/files/ii/secrets/front-proxy-ca.crt.enc")
        front-proxy-ca-key = filebase64("${path.module}/files/ii/secrets/front-proxy-ca.key.enc")
        front-proxy-client-crt = filebase64("${path.module}/files/ii/secrets/front-proxy-client.crt.enc")
        front-proxy-client-key = filebase64("${path.module}/files/ii/secrets/front-proxy-client.key.enc")
        kubelet-apiserver-client-crt = filebase64("${path.module}/files/ii/secrets/kubelet-apiserver-client.crt.enc")
        kubelet-apiserver-client-key = filebase64("${path.module}/files/ii/secrets/kubelet-apiserver-client.key.enc")
        sa-pub = filebase64("${path.module}/files/ii/secrets/sa.pub.enc")
        sa-key = filebase64("${path.module}/files/ii/secrets/sa.key.enc")
    }

    lifecycle {
        ignore_changes = [
            metadata.ssh-keys,
        ]
    }
}

resource "yandex_lb_target_group" "master_ig" {
  name      = "mk8s-master-targetgroup-v6"
  description = "target group for mk8s backend apiserver balancer"
  region_id = "ru-central1"

  target {
    subnet_id = ycp_compute_instance.mk8s-master.0.network_interface.0.subnet_id
    address   = ycp_compute_instance.mk8s-master.0.network_interface.0.primary_v6_address.0.address
  }

  target {
    subnet_id = ycp_compute_instance.mk8s-master.1.network_interface.0.subnet_id
    address   = ycp_compute_instance.mk8s-master.1.network_interface.0.primary_v6_address.0.address
  }

  target {
    subnet_id = ycp_compute_instance.mk8s-master.2.network_interface.0.subnet_id
    address   = ycp_compute_instance.mk8s-master.2.network_interface.0.primary_v6_address.0.address
  }
}
