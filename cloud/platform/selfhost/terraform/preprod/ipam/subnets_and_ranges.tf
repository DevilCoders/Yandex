data "yandex_vpc_subnet" "a" {
  subnet_id = "bucpba0hulgrkgpd58qp"
}

data "yandex_vpc_subnet" "b" {
  subnet_id = "bltueujt22oqg5fod2se"
}

data "yandex_vpc_subnet" "c" {
  subnet_id = "fo27jfhs8sfn4u51ak2s"
}

locals {
  zone_a_v6_range = data.yandex_vpc_subnet.a.v6_cidr_blocks[0]
  zone_a_v4_range = data.yandex_vpc_subnet.a.v4_cidr_blocks[0]

  zone_b_v6_range = data.yandex_vpc_subnet.b.v6_cidr_blocks[0]
  zone_b_v4_range = data.yandex_vpc_subnet.b.v4_cidr_blocks[0]

  zone_c_v6_range = data.yandex_vpc_subnet.c.v6_cidr_blocks[0]
  zone_c_v4_range = data.yandex_vpc_subnet.c.v4_cidr_blocks[0]
}

