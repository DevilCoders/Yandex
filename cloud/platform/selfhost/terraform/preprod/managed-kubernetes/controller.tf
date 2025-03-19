locals {
    blue_ipv4_addrs = [
        "172.16.13.88",
        "172.17.13.88",
        "172.18.13.88",
    ]

    blue_ipv6_addrs = [
        "2a02:6b8:c0e:501:0:f806:0:1388",
        "2a02:6b8:c02:901:0:f806:0:1388",
        "2a02:6b8:c03:501:0:f806:0:1388",
    ]

    green_ipv4_addrs = [
        "172.16.14.88",
        "172.17.14.88",
        "172.18.14.88",
    ]

    green_ipv6_addrs = [
        "2a02:6b8:c0e:501:0:f806:0:1488",
        "2a02:6b8:c02:901:0:f806:0:1488",
        "2a02:6b8:c03:501:0:f806:0:1488",
    ]

    controller_secondary_disk_size = 64

    blue_image_id = "fdvrsqmlhvk4l4oituos"
    green_image_id = "fdvrsqmlhvk4l4oituos"

    blue_name = "mk8s-controller-blue-preprod"
    green_name = "mk8s-controller-green-preprod"

    controller_conductor_group = "mk8s-controller"
    blue_controller_conductor_role = "blue"
    green_controller_conductor_role = "green"
    controller_osquery_tag = "ycloud-svc-mk8s-controller"

    controller_description = "Managed Kubernetes Controller"

    controller_count = 3
}

data "template_file" "mk8s-controller-blue-name" {
  count    = local.controller_count
  template = "$${name}"

  vars = {
    name = "${local.blue_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
  }
}

data "template_file" "mk8s-controller-blue-fqdn" {
  count    = local.controller_count
  template = "$${name}.$${domain}"

  vars = {
    name = "${local.blue_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
    domain = local.domain
  }
}

data "template_file" "mk8s-controller-green-name" {
  count    = local.controller_count
  template = "$${name}"

  vars = {
    name = "${local.green_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
  }
}

data "template_file" "mk8s-controller-green-fqdn" {
  count    = local.controller_count
  template = "$${name}.$${domain}"

  vars = {
    name = "${local.green_name}-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"
    domain = local.domain
  }
}

resource "ycp_compute_disk" "mk8s-controller-blue-disks" {
    count = local.controller_count

    name = "${local.blue_name}-disk-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"

    type_id = "network-ssd"
    size = local.controller_secondary_disk_size
    zone_id = element(local.zones, count.index % 3)
}

resource "ycp_compute_disk" "mk8s-controller-green-disks" {
    count = local.controller_count

    name = "${local.green_name}-disk-${element(local.datacenters, count.index % 3)}-${floor(count.index / 3) + 1}"

    type_id = "network-ssd"
    size = local.controller_secondary_disk_size
    zone_id = element(local.zones, count.index % 3)
}

data "template_file" "mk8s_controller_config_blue_ii" {
  template = file("${path.module}/files/ii/controller/mk8s-controller-blue.tpl.yaml")

  vars = {
    registry_auth = local.docker_auth
    juggler_token = local.juggler_token
    solomon_token = local.solomon_token
  }
}

data "template_file" "mk8s_controller_config_green_ii" {
  template = file("${path.module}/files/ii/controller/mk8s-controller-green.tpl.yaml")

  vars = {
    registry_auth = local.docker_auth
    juggler_token = local.juggler_token
    solomon_token = local.solomon_token
  }
}

resource "ycp_compute_instance" "mk8s-controller-blue" {
    allow_stopping_for_update = true
    stop_for_metadata_update = true
    count = local.controller_count

    platform_id   = "standard-v1"

    resources {
        cores         = 2
        memory        = 4
        core_fraction = 20
    }

    description = local.controller_description
    service_account_id  = "bfb4sriefpc62cd522k7"

    boot_disk {
        disk_spec {
            type_id = "network-hdd"
            size = 32
            image_id = local.blue_image_id
        }
    }

    zone_id = element(local.zones, count.index % 3)

    network_interface {
        subnet_id    = element(local.subnets, count.index % 3)
        primary_v4_address {
            address = element(local.blue_ipv4_addrs, count.index)
        }
        primary_v6_address {
            address = element(local.blue_ipv6_addrs, count.index)
        }
    }

    secondary_disk {
      auto_delete = false
      disk_id = ycp_compute_disk.mk8s-controller-blue-disks[count.index].id
    }

    provisioner "local-exec" {
        when        = destroy
        command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
        environment = {}

        on_failure = continue
    }

    name     = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)
    hostname = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        env = local.environment
        environment = local.environment
        conductor-group = local.controller_conductor_group
        conductor-role = local.blue_controller_conductor_role
        yandex-dns = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)
        layer = "paas"
    }

    metadata = {
        user-data = file("${path.module}/files/ii/controller/user-data.yaml")
        osquery_tag = local.controller_osquery_tag
        serial-port-enable = "1"
        shortname = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)
        internal-name = element(data.template_file.mk8s-controller-blue-name.*.rendered, count.index)
        
        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/controller/bootstrap.yaml")
        skm-config = file("${path.module}/files/ii/controller/skm.yaml")
        base64-decoder-sh = file("${path.module}/files/ii/controller/base64decode_secrets.sh")
        setup-kubectl-sh = file("${path.module}/files/ii/controller/setup_kubectl.sh")

        metricsagent-config = file("${path.module}/files/ii/controller/metricsagent.yaml")

        yc-config = file("${path.module}/files/ii/controller/yc.yaml")
        ycp-config = file("${path.module}/files/ii/controller/ycp.yaml")
        controller-config = data.template_file.mk8s_controller_config_blue_ii.rendered

        registry-auth = filebase64("${path.module}/files/ii/controller/secrets/docker_config.json.enc")
        metricsagent-auth = filebase64("${path.module}/files/ii/controller/secrets/solomon_oauth_token.enc")

        kubeconfig = filebase64("${path.module}/files/ii/controller/secrets/kubeconfig.enc")
        service-sa-key = filebase64("${path.module}/files/ii/controller/secrets/yc-sa-key.json.enc")
        addons-s3-editor = filebase64("${path.module}/files/ii/controller/secrets/addon-s3-editor.json.enc")
        addons-s3-viewer = filebase64("${path.module}/files/ii/controller/secrets/addon-s3-viewer.json.enc")
        cr-yandex-sa-key = filebase64("${path.module}/files/ii/controller/secrets/cr-yandex-sa-key.json.enc")
        deks-initial = filebase64("${path.module}/files/ii/controller/secrets/dek_initial.enc")
    }
}

resource "ycp_compute_instance" "mk8s-controller-green" {
    count = local.controller_count
    allow_stopping_for_update = true
    stop_for_metadata_update = true

    platform_id   = "standard-v1"

    resources {
        cores         = 2
        memory        = 8
        core_fraction = 20
    }

    description = local.controller_description
    service_account_id  = "bfb4sriefpc62cd522k7"

    boot_disk {
        disk_spec {
            type_id = "network-hdd"
            size = 32
            image_id = local.green_image_id
        }
    }

    zone_id = element(local.zones, count.index % 3)

    network_interface {
        subnet_id    = element(local.subnets, count.index % 3)
        primary_v4_address {
            address = element(local.green_ipv4_addrs, count.index)
        }
        primary_v6_address {
            address = element(local.green_ipv6_addrs, count.index)
        }
    }

    secondary_disk {
      auto_delete = false
      disk_id = ycp_compute_disk.mk8s-controller-green-disks[count.index].id
    }

    provisioner "local-exec" {
        when        = destroy
        command     = "${path.module}/../../common/juggler-downtime.sh on ${self.network_interface.0.primary_v6_address.0.address}"
        environment = {}

        on_failure = continue
    }

    name     = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)
    hostname = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)

    labels = {
        abc_svc = local.abc_svc
        env = local.environment
        environment = local.environment
        conductor-group = local.controller_conductor_group
        conductor-role = local.green_controller_conductor_role
        yandex-dns = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)
        layer = "paas"
    }

    metadata = {
        user-data = file("${path.module}/files/ii/controller/user-data.yaml")
        osquery_tag = local.controller_osquery_tag
        serial-port-enable = "1"
        shortname = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)
        nsdomain = local.domain
        internal-hostname = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)
        internal-name = element(data.template_file.mk8s-controller-green-name.*.rendered, count.index)
        
        ssh-keys = module.ssh-keys.ssh-keys

        k8s-runtime-bootstrap-yaml = file("${path.module}/files/ii/controller/bootstrap.yaml")
        skm-config = file("${path.module}/files/ii/controller/skm.yaml")
        base64-decoder-sh = file("${path.module}/files/ii/controller/base64decode_secrets.sh")
        setup-kubectl-sh = file("${path.module}/files/ii/controller/setup_kubectl.sh")

        metricsagent-config = file("${path.module}/files/ii/controller/metricsagent.yaml")

        yc-config = file("${path.module}/files/ii/controller/yc.yaml")
        ycp-config = file("${path.module}/files/ii/controller/ycp.yaml")
        controller-config = data.template_file.mk8s_controller_config_green_ii.rendered

        registry-auth = filebase64("${path.module}/files/ii/controller/secrets/docker_config.json.enc")
        metricsagent-auth = filebase64("${path.module}/files/ii/controller/secrets/solomon_oauth_token.enc")

        kubeconfig = filebase64("${path.module}/files/ii/controller/secrets/kubeconfig.enc")
        service-sa-key = filebase64("${path.module}/files/ii/controller/secrets/yc-sa-key.json.enc")
        addons-s3-editor = filebase64("${path.module}/files/ii/controller/secrets/addon-s3-editor.json.enc")
        addons-s3-viewer = filebase64("${path.module}/files/ii/controller/secrets/addon-s3-viewer.json.enc")
        cr-yandex-sa-key = filebase64("${path.module}/files/ii/controller/secrets/cr-yandex-sa-key.json.enc")
        deks-initial = filebase64("${path.module}/files/ii/controller/secrets/dek_initial.enc")
    }
}
