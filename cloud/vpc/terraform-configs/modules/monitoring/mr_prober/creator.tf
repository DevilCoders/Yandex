resource "yandex_compute_image" "creator" {
  folder_id = var.folder_id

  name = "creator"
  description = "Image for Creator service built with packer from Mr.Prober images collection"
  source_url = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/creator/fd85lrkto671enf9hrrl.qcow2"

  timeouts {
    create = "10m"
  }
}

resource "yandex_vpc_security_group" "creator" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.mr_prober_control.id
  name = "creator-sg"
  description = "Security group for creator"

  ingress {
    protocol = "TCP"
    description = "Allow SSH"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "ICMP"
    description = "Allow pings"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port = "0"
  }

  ingress {
    protocol = "IPV6_ICMP"
    description = "Allow pings"
    v6_cidr_blocks = ["::/0"]
    port = "0"
  }

  dynamic "ingress" {
    for_each = toset([8080, 16300, 22132])
    content {
      protocol = "TCP"
      description = "Allow ${ingress.key}/tcp from yandex: it's solomon-agent/unified-agent port"
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/32"]
      port = ingress.key
    }
  }

  egress {
    protocol = "ANY"
    description = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port = 0
    to_port = 65535
  }
}

data "external" "creator_skm_metadata" {
  program = [
    "python3", "${path.module}/../../shared/skm/generate.py"
  ]
  query = {
    secrets_file = "${abspath(path.module)}/creator_secrets.yaml",

    cache_dir = "${path.root}/.skm-cache/mr-prober-${var.mr_prober_environment}/",
    ycp_profile = var.ycp_profile,
    yc_endpoint = var.yc_endpoint,
    mr_prober_environment = var.mr_prober_environment,
    s3_stand = var.s3_stand,
    key_uri = "yc-kms://${yandex_kms_symmetric_key.mr_prober_secret_kek.id}"
  }
}

# Creator is fixed-size instance group (of size 1)
# It's not a simple instance, i.e. because instance group will re-create the instance
# if metadata is changed
resource "ycp_microcosm_instance_group_instance_group" "creator" {
  count = var.run_creator ? 1 : 0

  folder_id = var.folder_id

  name = "creator"
  description = "Creator service"

  service_account_id = yandex_iam_service_account.mr_prober_sa.id

  instance_template {
    boot_disk {
      disk_spec {
        type_id = "network-ssd"
        image_id = yandex_compute_image.creator.id
        size = var.creator_disk_size
      }
    }

    resources {
      memory = var.creator_vm_memory
      cores = var.creator_vm_cores
      core_fraction = var.creator_vm_fraction
    }

    network_interface {
      network_id = ycp_vpc_network.mr_prober_control.id
      subnet_ids = [lookup(local.control_network_subnet_ids, var.creator_zone_id)]
      security_group_ids = [yandex_vpc_security_group.creator.id]

      primary_v4_address {}
      primary_v6_address {
        dns_record_spec {
          fqdn = "creator.${var.dns_zone}."
          dns_zone_id = ycp_dns_dns_zone.mr_prober.id
          ptr = true
        }
      }
    }

    name = "creator-{instance.index}"
    description = "Creator service"
    hostname = "{instance.index}.creator.${var.dns_zone}"
    fqdn = "{instance.index}.creator.${var.dns_zone}"

    metadata = {
      user-data = templatefile(
        "${path.module}/cloud-init.yaml",
        {
          hostname = "creator.${var.dns_zone}",
          s3_endpoint = var.s3_endpoint,
          stand_name = var.mr_prober_environment,
          grpc_iam_api_endpoint = var.grpc_iam_api_endpoint,
          grpc_compute_api_endpoint = var.grpc_compute_api_endpoint,
          api_domain = var.api_domain,
          meeseeks_compute_node_prefixes_cli_param = var.meeseeks_compute_node_prefixes_cli_param
        }
      )
      enable-oslogin = "true"
      skm = data.external.creator_skm_metadata.result.skm
    }

    labels = {
      layer = "iaas"
      abc_svc = "ycvpc"
      env = var.environment
    }

    platform_id = var.platform_id

    service_account_id = yandex_iam_service_account.mr_prober_sa.id
  }

  allocation_policy {
    zone {
      zone_id = var.creator_zone_id
    }
  }

  deploy_policy {
    max_unavailable = 1
    max_creating = 1
    max_expansion = 0
    max_deleting = 1
  }

  scale_policy {
    fixed_scale {
      size = 1
    }
  }

  depends_on = [
    ycp_resource_manager_folder_iam_member.mr_prober_sa
  ]
}

resource "ytr_conductor_group" "creator" {
  count = var.use_conductor ? 1 : 0
  name = "cloud_${var.mr_prober_environment}_mr_prober_creator"
  project_id = data.ytr_conductor_project.cloud.id
  description = "Creator instances of Mr. Prober"
  parent_group_ids = [
    ytr_conductor_group.mr_prober[0].id
  ]
}

resource "ytr_conductor_host" "creator" {
  count = var.use_conductor ? 1 : 0
  fqdn = "creator.${var.dns_zone}"
  short_name = "creator.${var.dns_zone}"
  datacenter_id = lookup(local.ytr_conductor_datacenter_by_zone, var.creator_zone_id)
  group_id = ytr_conductor_group.creator[0].id
}
