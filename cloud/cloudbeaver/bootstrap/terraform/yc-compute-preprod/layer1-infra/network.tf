data "ycp_vpc_network" "cloudbeaver" {
  network_id = "c6453k00df51004snnkd"
}

data "ycp_vpc_subnet" "ru-central1-a" {
  subnet_id = "bucj9i1cqha4s8kuh1ma"
}

data "ycp_vpc_subnet" "ru-central1-b" {
  subnet_id = "bltft2fb5ve5b1pp3cao"
}

data "ycp_vpc_subnet" "ru-central1-c" {
  subnet_id = "fo2351l335nrj03al203"
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
