resource "yandex_compute_image" "common-2if" {
  family     = "common-2if"
  source_url = "https://storage.yandexcloud.net/a2bd1a42-69fd-451a-8dee-47ba04fbacbf/dbaas-common-2if-image-2020-07-29-1595989163.img"
}

resource "yandex_compute_image" "common-1if" {
  family     = "common-1if"
  source_url = "https://storage.yandexcloud.net/a2bd1a42-69fd-451a-8dee-47ba04fbacbf/dbaas-common-1if-image-2020-07-27-1595814831.img"
}

resource "ycp_compute_instance" "mdb-bootstrap01k" {
  name        = "mdb-bootstrap01k"
  zone_id     = "ru-central1-a"
  platform_id = "standard-v2"
  fqdn        = "mdb-bootstrap01k.yandexcloud.net"
  description = "one interface, no interconnect"

  resources {
    core_fraction = 50
    cores         = 4
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = yandex_compute_image.common-1if.id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.cloud-mdb-gpn-appendix-prod-nets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-bootstrap01h" {
  name        = "mdb-bootstrap01h"
  zone_id     = "ru-central1-b"
  platform_id = "standard-v2"
  fqdn        = "mdb-bootstrap01h.yandexcloud.net"

  resources {
    core_fraction = 50
    cores         = 4
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = yandex_compute_image.common-2if.id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.cloud-mdb-gpn-appendix-prod-nets-ru-central1-b.id
    primary_v6_address {}
  }

  network_interface {
    subnet_id = data.yandex_vpc_subnet.interconnect-nets-ru-central1-b.id
    primary_v6_address {}
  }
}
