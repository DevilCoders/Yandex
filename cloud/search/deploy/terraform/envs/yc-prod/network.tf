resource "ycp_vpc_network" "yc-search-prod-nets" {
  lifecycle {
    prevent_destroy = true
  }
  name      = "yc-search-prod-nets"
  folder_id = var.folder_id
}

resource "ycp_vpc_subnet" "yc-search-prod-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = true
  }
  name           = "yc-search-prod-nets-ru-central1-b"
  network_id     = ycp_vpc_network.yc-search-prod-nets.id
  zone_id        = "ru-central1-b"
  v6_cidr_blocks = ["2a02:6b8:c02:900:0:f85e:8ce5:0/112"]
  v4_cidr_blocks = ["172.17.0.0/16"]
}

resource "ycp_vpc_subnet" "yc-search-prod-nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  name           = "yc-search-prod-nets-ru-central1-a"
  network_id     = ycp_vpc_network.yc-search-prod-nets.id
  zone_id        = "ru-central1-a"
  v6_cidr_blocks = ["2a02:6b8:c0e:500:0:f85e:8ce5:0/112"]
  v4_cidr_blocks = ["172.16.0.0/16"]
}

resource "ycp_vpc_subnet" "yc-search-prod-nets-ru-central1-c" {
  lifecycle {
    prevent_destroy = true
  }
  name           = "yc-search-prod-nets-ru-central1-c"
  network_id     = ycp_vpc_network.yc-search-prod-nets.id
  v6_cidr_blocks = ["2a02:6b8:c03:500:0:f85e:8ce5:0/112"]
  v4_cidr_blocks = ["172.18.0.0/16"]
  zone_id        = "ru-central1-c"
}
