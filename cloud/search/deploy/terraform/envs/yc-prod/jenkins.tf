resource "ycp_compute_instance" "ycsearch-jenkins01k" {
  name        = "ycsearch-jenkins01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "ycsearch-jenkins01k.yandexcloud.net"

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size     = 150
      image_id = "fd8bjo6n25689s5shfbi" #  yc --profile search-prod-sa compute image get-latest-from-family yc-search-base-1804
      type_id  = "network-ssd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.yc-search-prod-nets-ru-central1-a.id
    primary_v6_address {}
  }
}
