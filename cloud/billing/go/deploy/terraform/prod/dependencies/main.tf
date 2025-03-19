terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}

variable "yc_folder" {}

# DNS zone for VMs
data "yandex_dns_zone" "billing_dns" {
  dns_zone_id = "dns6diqquoe59a4t6lle"
}

output "nsdomain" {
  value = trimsuffix(data.yandex_dns_zone.billing_dns.zone, ".")
}

output "dns_zone" {
  value = {
    id   = data.yandex_dns_zone.billing_dns.id
    zone = data.yandex_dns_zone.billing_dns.zone
  }
}

# SA for VM
data "yandex_iam_service_account" "vm_sa" {
  name = "yc-billing-svm"
}

output "sa" {
  value = {
    id   = data.yandex_iam_service_account.vm_sa.id
    name = data.yandex_iam_service_account.vm_sa.name
  }
}

# Billling folder nets
data "yandex_vpc_subnet" "piper_subnets" {
  for_each = toset([
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ])

  folder_id = var.yc_folder
  name      = "billing-nets-${each.key}"
}

output "subnets" {
  value = { for k, v in data.yandex_vpc_subnet.piper_subnets :
    k => {
      id   = v.id
      name = v.name
      zone = v.zone
    }
  }
}

data "yandex_vpc_security_group" "piper" {
  folder_id = var.yc_folder
  name      = "piper-security-group"
}

output "piper-security-group" {
  value = {
    id   = data.yandex_vpc_security_group.piper.id
    name = data.yandex_vpc_security_group.piper.name
  }
}
