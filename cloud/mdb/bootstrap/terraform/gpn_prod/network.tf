data "ycp_vpc_network" "mdb-control-plane-nets" {
  network_id = "cgmtp8p7g367f9gea2ge"
}

data "ycp_vpc_subnet" "mdb-control-plane-nets-ru-gpn-spb99" {
  subnet_id = "e575vad099cva988qle1"
}

data "ycp_vpc_network" "mdb-e2e-nets" {
  network_id = "cgmss7sj0h1aa26keaot"
}

data "ycp_vpc_subnet" "mdb-e2e-nets-ru-gpn-spb99" {
  subnet_id = "e57q5du4bilu0jhfo2jm"
}
