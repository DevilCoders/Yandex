// docs https://wiki.yandex-team.ru/cloud/devel/selfhost/duty/buildagents/
// export YC_TOKEN=$(yc --profile=preprod-tca iam create-token)
// terraform init -backend-config="secret_key=$(ya vault get version ver-01e9d91yf5qmcq891em025rwgr -o s3_secret_access_key 2>/dev/null)"
// terraform plan

locals {
  dc_zone_id_map = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
  }
  build_agents = {
    for build_agent in flatten([
      [
        for dc, count in var.build_agents_per_dc : {
          name : format("cm-build-agent-%s%d", dc, 1),
          dc : format("%s", dc),
          zone_id : local.dc_zone_id_map[dc]
        }
      ],
      [for dc, count in var.build_agents_per_dc : [
        for i in range(1, count + 1) : {
          name : format("build-agent-%s%d", dc, i),
          dc : format("%s", dc),
          zone_id : local.dc_zone_id_map[dc]
        }
      ]]
    ]) : build_agent.name => build_agent
  }
  dns_zone_id = "aet5iocohvngcfg61d6a"
  dns_zone    = "bootstrap.cloud-preprod.yandex.net"
  folder_id   = data.terraform_remote_state.preprod-bootstrap-resources.outputs.yc-build-agents-folder-id
  subnet_ids  = data.terraform_remote_state.preprod-bootstrap-resources.outputs.yc-build-agents-subnet-ids
  snapshot_id = var.snapshot_id
  image_id    = var.boot_disk_id
}


data "terraform_remote_state" "preprod-bootstrap-resources" {
  backend = "http"

  config = {
    address = "https://s3.mds.yandex.net/yc-bootstrap-lts/terraform/preprod/bootstrap"
  }
}

data "ycp_compute_image" "build-agent-image" {
  image_id  = local.image_id
  folder_id = local.folder_id
}

resource "ycp_compute_disk" "build-agent-data-disk" {
  for_each    = local.build_agents
  size        = 100
  type_id     = "network-ssd"
  zone_id     = each.value.zone_id
  folder_id   = local.folder_id
  snapshot_id = local.snapshot_id
  disk_placement_policy {}
  lifecycle {
    ignore_changes = [labels]
  }
}

resource "ycp_compute_instance" "build-agents" {
  for_each        = local.build_agents
  name            = each.key
  zone_id         = each.value.zone_id
  cauth           = false
  folder_id       = local.folder_id
  platform_id     = "standard-v2"
  pci_topology_id = "V2"
  labels          = {}

  resources {
    cores      = 8
    memory     = 32
    gpus       = 0
    nvme_disks = 0
  }

  metadata = {
    user-data = templatefile("${path.module}/user-data.tmpl", {
      fqdn : "${each.key}.${local.dns_zone}"
    })
    skm            = file("${path.module}/files/skm-md.yaml")
    enable-oslogin = true
  }

  boot_disk {
    mode = "READ_WRITE"
    disk_spec {
      size     = 150
      labels   = {}
      image_id = data.ycp_compute_image.build-agent-image.id
      type_id  = "network-ssd"
    }
  }

  service_account_id = ycp_iam_service_account.sa-build-agents.id

  secondary_disk {
    auto_delete = false
    disk_id     = ycp_compute_disk.build-agent-data-disk[each.key].id
  }
  nested_virtualization = false
  network_interface {
    subnet_id          = local.subnet_ids[each.value.zone_id].id
    security_group_ids = []
    primary_v4_address {}
    primary_v6_address {
      dns_record {
        fqdn        = each.key
        dns_zone_id = local.dns_zone_id
        ptr         = true
        ttl         = 900
      }
      dns_record {
        fqdn        = "${each.value.dc}.build-agents"
        dns_zone_id = local.dns_zone_id
        ptr         = false
        ttl         = 900
      }
    }

  }

  lifecycle {
    ignore_changes = [
      boot_disk[0].disk_spec[0].labels,
    ]
  }
}

resource "null_resource" "provision-build-agents" {
  for_each = local.build_agents
  provisioner "remote-exec" {
    connection {
      type = "ssh"
      host = ycp_compute_instance.build-agents[each.key].network_interface[0].primary_v6_address[0].address


      agent        = var.ssh_agent
      user         = var.ssh_user
      private_key  = var.ssh_private_key != "" ? file(var.ssh_private_key) : null
      timeout      = "30m"
      bastion_host = var.bastion_host
    }
    scripts = ["${path.module}/files/wait_cloud_init.sh", "${path.module}/files/provision-build-agent.sh"]
  }
  triggers = {
    instance_id = ycp_compute_instance.build-agents[each.key].id
  }
}

resource "ycp_iam_service_account" "sa-build-agents" {
  name               = "sa-vpc-build-agents-preprod"
  service_account_id = "yc.vpc.build-agents-sa"
  folder_id          = local.folder_id
  description        = "Used in build-agents instances"
  lifecycle {
    prevent_destroy = true
  }
}
