locals {
  service_map = {
    "api-router" = 10
    "ai-gateway" = 11
    "atlantis"   = 11
  }
}

data "template_file" "service-subnets-net-id-a" {
  count = length(local.service_map)

  template = "$${service_net_id}"

  vars = {
    service_net_id = lookup(
      local.service_map,
      element(keys(local.service_map), count.index),
      "some-default",
    )
  }
}

data "template_file" "for-render-cidr" {
  count = length(local.service_map)

  template = cidrsubnet(
    local.zone_c_v6_range,
    8,
    element(
      data.template_file.service-subnets-net-id-a.*.rendered,
      count.index,
    ),
  )
}

locals {
  service_api-router_net-id = local.service_map["api-router"]
  service_api-router_v6_cidr_list = [
    cidrsubnet(local.zone_a_v6_range, 24, local.service_api-router_net-id),
    cidrsubnet(local.zone_b_v6_range, 24, local.service_api-router_net-id),
    cidrsubnet(local.zone_c_v6_range, 24, local.service_api-router_net-id),
  ]
  service_api-router_v4_cidr_list = [
    cidrsubnet(local.zone_a_v4_range, 8, local.service_api-router_net-id),
    cidrsubnet(local.zone_b_v4_range, 8, local.service_api-router_net-id),
    cidrsubnet(local.zone_c_v4_range, 8, local.service_api-router_net-id),
  ]

  service_atlantis_net-id = local.service_map["atlantis"]
  service_atlantis_v6_cidr_list = [
    cidrsubnet(local.zone_a_v6_range, 24, local.service_atlantis_net-id),
    cidrsubnet(local.zone_b_v6_range, 24, local.service_atlantis_net-id),
    cidrsubnet(local.zone_c_v6_range, 24, local.service_atlantis_net-id),
  ]
  service_atlantis_v4_cidr_list = [
    cidrsubnet(local.zone_a_v4_range, 8, local.service_atlantis_net-id),
    cidrsubnet(local.zone_b_v4_range, 8, local.service_atlantis_net-id),
    cidrsubnet(local.zone_c_v4_range, 8, local.service_atlantis_net-id),
  ]
}

data "template_file" "service-api-router-v6" {
  count = var.address_count_produce
  template = cidrhost(
    element(local.service_api-router_v6_cidr_list, count.index),
    floor(1 + count.index / length(var.zone_suffix_list)),
  )
}

data "template_file" "service-api-router-v4" {
  count = var.address_count_produce
  template = cidrhost(
    element(local.service_api-router_v4_cidr_list, count.index),
    floor(1 + count.index / length(var.zone_suffix_list)),
  )
}

data "template_file" "service-atlantis-v6" {
  count = var.address_count_produce
  template = cidrhost(
    element(local.service_atlantis_v6_cidr_list, count.index),
    floor(1 + count.index / length(var.zone_suffix_list)),
  )
}

data "template_file" "service-atlantis-v4" {
  count = var.address_count_produce
  template = cidrhost(
    element(local.service_atlantis_v4_cidr_list, count.index),
    floor(1 + count.index / length(var.zone_suffix_list)),
  )
}

