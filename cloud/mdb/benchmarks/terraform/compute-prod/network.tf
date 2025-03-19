resource "ycp_vpc_network" "mdbtanksprodnets" {
  name = "mdb-tanks-prod-nets"
}

resource "ycp_vpc_subnet" "mdb-tanks-prod-nets-ru-central1-a" {
  name           = "mdb-tanks-prod-nets-ru-central1-a"
  network_id     = ycp_vpc_network.mdbtanksprodnets.id
  v4_cidr_blocks = ["172.16.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c0e:500:0:f848:fe0b:0/112"]
}

resource "ycp_vpc_subnet" "mdb-tanks-prod-nets-ru-central1-b" {
  name           = "mdb-tanks-prod-nets-ru-central1-b"
  network_id     = ycp_vpc_network.mdbtanksprodnets.id
  v4_cidr_blocks = ["172.17.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c02:900:0:f848:fe0b:0/112"]
}

resource "ycp_vpc_subnet" "mdb-tanks-prod-nets-ru-central1-c" {
  name           = "mdb-tanks-prod-nets-ru-central1-c"
  network_id     = ycp_vpc_network.mdbtanksprodnets.id
  v4_cidr_blocks = ["172.18.0.0/16"]
  v6_cidr_blocks = ["2a02:6b8:c03:500:0:f848:fe0b:0/112"]
}
