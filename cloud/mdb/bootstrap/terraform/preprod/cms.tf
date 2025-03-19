module "cmsgrpcapi" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "cmsgrpcapi"

  resources = {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fdv10th24ce4ia4kg679"
    type_id  = "network-hdd"
  }


  override_boot_disks = {
    "01.zone_b" = {
      size     = 30
      image_id = "fdv2sbduod8qobb2vm47"
      type_id  = "network-hdd"
    }
  }

  override_load_balancer_ipv6_addr = "2a0d:d6c0:0:ff1a::1c4"
  create_load_balancer             = true
}

module "cmsdb" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "cmsdb"

  resources = {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fdv10th24ce4ia4kg679"
    type_id  = "network-hdd"
  }

  override_boot_disks = {
    "01.zone_b" = {
      size     = 30
      image_id = "fdv19ubn2bdfq62v5mop"
      type_id  = "network-hdd"
    }
  }
}

resource "ycp_iam_service_account" "mdb-cms" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-cms"
  description        = "CMS for compute instances"
  service_account_id = "yc.mdb.cms"
}

module "cms-autoduty" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "cms-autoduty"

  resources = {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fdvfoo698ce0n7rttgc2"
    type_id  = "network-hdd"
  }

  override_boot_disks = {
    "01.zone_b" = {
      size     = 30
      image_id = "fdv2sbduod8qobb2vm47"
      type_id  = "network-hdd"
    }
  }
}
