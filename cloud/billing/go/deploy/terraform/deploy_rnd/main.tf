locals {
  base_image_family      = "paas-base-g4"
  paas_images_folder     = "b1grpl1006mpj1jtifi1"
  billing_service_folder = "yc.billing.service-folder"
  sa_name                = "sa-rnd"
  canary_name            = "pass-base-canary"

  dns_zone_id = "dns6diqquoe59a4t6lle"
}

# DNS zone for VMs
data "yandex_dns_zone" "billing_dns" {
  dns_zone_id = local.dns_zone_id
}

locals {
  nsdomain = trimsuffix(data.yandex_dns_zone.billing_dns.zone, ".")
}

output "dns_zone" {
  value = data.yandex_dns_zone.billing_dns
}

# Select base image for canary
data "yandex_compute_image" "paas_image" {
  family    = local.base_image_family
  folder_id = local.paas_images_folder
}

output "base_image" {
  value = data.yandex_compute_image.paas_image
}

# Billling folder nets
data "yandex_vpc_subnet" "billing_subnet" {
  folder_id = local.billing_service_folder
  name      = "billing-nets-${var.yc_zone}"
}

output "subnet" {
  value = data.yandex_vpc_subnet.billing_subnet
}

# SA for VM
data "yandex_iam_service_account" "vm_sa" {
  name = local.sa_name
}

output "sa" {
  value = data.yandex_iam_service_account.vm_sa
}

module "pass-base-canary" {
  source               = "../modules/base_instance"
  installation         = "default"
  name                 = local.canary_name
  instance_description = "canary host with base paas image"
  folder_id            = var.yc_folder
  subnet               = data.yandex_vpc_subnet.billing_subnet.id
  # ssh-keys             = module.billing-abc-ssh.ssh-keys
  oslogin = true

  skip_underlay = true
  dns_zone_id   = local.dns_zone_id
  nsdomain      = local.nsdomain

  image_id           = data.yandex_compute_image.paas_image.image_id
  zone               = var.yc_zone
  service_account_id = data.yandex_iam_service_account.vm_sa.id
  security_group_ids = []
}

output "canary" {
  value = module.pass-base-canary
}
