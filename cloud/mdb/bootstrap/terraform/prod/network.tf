resource "ycp_vpc_network" "dbaasexternalnets" {
  name = "dbaasexternalnets"
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-a" {
  name           = "dbaasexternalnets-ru-central1-a"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c0e:500:0:f802::/96"]
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-b" {
  name           = "dbaasexternalnets-ru-central1-b"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c02:900:0:f802::/96"]
}

resource "ycp_vpc_subnet" "dbaasexternalnets-ru-central1-c" {
  name           = "dbaasexternalnets-ru-central1-c"
  network_id     = ycp_vpc_network.dbaasexternalnets.id
  v6_cidr_blocks = ["2a02:6b8:c03:500:0:f802::/96"]
}

resource "ycp_vpc_network" "dataproc-ui-proxy" {
  name = "cloud-dataproc-ui-proxy-prod-nets"
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1a" {
  name           = "cloud-dataproc-ui-proxy-prod-nets-ru-central1-a"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c0e:500:0:f846::/96"]
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1b" {
  name           = "cloud-dataproc-ui-proxy-prod-nets-ru-central1-b"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c02:900:0:f846::/96"]
}

resource "ycp_vpc_subnet" "dataproc-ui-proxy-rc1c" {
  name           = "cloud-dataproc-ui-proxy-prod-nets-ru-central1-c"
  network_id     = ycp_vpc_network.dataproc-ui-proxy.id
  v6_cidr_blocks = ["2a02:6b8:c03:500:0:f846::/96"]
}
