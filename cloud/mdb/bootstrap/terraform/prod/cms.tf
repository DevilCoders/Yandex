module "cmsgrpcapi" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "cmsgrpcapi"

  resources = {
    core_fraction = 20
    cores         = 2
    memory        = 2
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd8rohggmed6uuv98d77"
    type_id  = "network-hdd"
  }

  override_load_balancer_ipv6_addr = "2a0d:d6c0:0:1b::1d5"
  create_load_balancer             = true
}

module "cmsdb" {
  source = "../modules/instance/v1"

  installation = local.installation
  service_name = "cmsdb"

  resources = {
    core_fraction = 20
    cores         = 2
    memory        = 4
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd8rohggmed6uuv98d77"
    type_id  = "network-hdd"
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
    core_fraction = 20
    cores         = 2
    memory        = 2
  }

  boot_disk_spec = {
    size     = 30
    image_id = "fd8rohggmed6uuv98d77"
    type_id  = "network-hdd"
  }
}
