resource "ycp_compute_instance" "api-admin-dbaas-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "api-admin-dbaas-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "api-admin-dbaas-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fdv83lcfofc72p9o4j0m"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_iam_service_account" "internal-api" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "internal-api"
  service_account_id = "yc.mdb.internal-api"
}
