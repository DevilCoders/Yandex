resource "ycp_vpc_network" "dbaasexternalnets" {
  name = "dbaasexternalnets"
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-a" {
  name           = "dbaasexternalnets-ru-central1-a"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:f802::/96"]
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-b" {
  name           = "dbaasexternalnets-ru-central1-b"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:f802::/96"]
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-c" {
  name           = "dbaasexternalnets-ru-central1-c"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:f802::/96"]
}
