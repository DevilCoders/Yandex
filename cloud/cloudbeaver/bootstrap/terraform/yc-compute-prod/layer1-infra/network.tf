data "ycp_vpc_network" "cloudbeaver" {
  network_id = "enp99mom4p15mkcfh6v2"
}

data "ycp_vpc_subnet" "ru-central1-a" {
  subnet_id = "e9b2hi6c3pllesl32l1v"
}

data "ycp_vpc_subnet" "ru-central1-b" {
  subnet_id = "e2lvsqa7v5ile21qgpqt"
}

data "ycp_vpc_subnet" "ru-central1-c" {
  subnet_id = "b0cdrl0epfeum1t14r89"
}

locals {
  v4_cidrs = concat(
    data.ycp_vpc_subnet.ru-central1-a.v4_cidr_blocks,
    data.ycp_vpc_subnet.ru-central1-b.v4_cidr_blocks,
    data.ycp_vpc_subnet.ru-central1-c.v4_cidr_blocks,
  )
  v6_cidrs = concat(
    data.ycp_vpc_subnet.ru-central1-a.v6_cidr_blocks,
    data.ycp_vpc_subnet.ru-central1-b.v6_cidr_blocks,
    data.ycp_vpc_subnet.ru-central1-c.v6_cidr_blocks,
  )
  k8s_locations = [
    {
      subnet_id = data.ycp_vpc_subnet.ru-central1-a.id
      zone      = data.ycp_vpc_subnet.ru-central1-a.zone_id
    },
    {
      subnet_id = data.ycp_vpc_subnet.ru-central1-b.id
      zone      = data.ycp_vpc_subnet.ru-central1-b.zone_id
    },
    {
      subnet_id = data.ycp_vpc_subnet.ru-central1-c.id
      zone      = data.ycp_vpc_subnet.ru-central1-c.zone_id
    }
  ]
}
