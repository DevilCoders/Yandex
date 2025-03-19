/*
 * data_sources/networks - contains exported datasources data in suitable format
 * for usage with standard_service && standard_instance_group modules
 * each output entry represent everything needed for configuration of ONE network interface 
 *
 * each output variable should have format
 * {
 *   network = string        :: id of VPC network interface will be assosiated with
 *   subnets = map(string)   :: zone => subnet_id map (each subnet should be assosiated with network above)
 *   ipv4 = bool             :: should interface provide ipv4 address
 *   ipv6 = bool             :: should interface provide ipv6 address
 * }
 *
 * TODO: Currenlty is not possible to check is there ipv4 or ipv6 enabled
 *       using yc terraform provider, probably will be fixed in future releases
 */

locals {
  is_prod = var.environment == "preprod" ? false : true
}

# cloud-preprod - preprod DS: user-nets 
data "yandex_vpc_network" "preprod_network_user_nets" {
  count      = local.is_prod ? 0 : 1
  network_id = "c64hcslsgv550q4kfvvk"
}
data "yandex_vpc_subnet" "preprod_subnets_user_nets" {
  for_each = local.is_prod ? toset([]) : toset(
    data.yandex_vpc_network.preprod_network_user_nets[0].subnet_ids
  )
  subnet_id = each.key
}
locals {
  preprod_user_nets = local.is_prod ? null : {
    network = data.yandex_vpc_network.preprod_network_user_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.preprod_subnets_user_nets :
      subnet.zone => subnet.subnet_id
      # No filtering
    }
    ipv4 = true
    ipv6 = false
  }
}

# cloud-prod - staging DS: user-nets 
data "yandex_vpc_network" "staging_network_user_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enp2upi01jrmjlqe2oa3"
}
data "yandex_vpc_subnet" "staging_subnets_user_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.staging_network_user_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  staging_user_nets = local.is_prod ? {
    network = data.yandex_vpc_network.staging_network_user_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.staging_subnets_user_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("zone", subnet.name)) > 0
      # Filtering because of dataproc-nets in the same network
    }
    ipv4 = true
    ipv6 = false
  } : null
}

# cloud-prod - prod DS: user-nets 
data "yandex_vpc_network" "prod_network_user_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enppnvgvk5vsc6ceh7lb"
}
data "yandex_vpc_subnet" "prod_subnets_user_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.prod_network_user_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  prod_user_nets = local.is_prod ? {
    network = data.yandex_vpc_network.prod_network_user_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.prod_subnets_user_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("zone", subnet.name)) > 0
      # Filtering because of dataproc-nets in the same network
    }
    ipv4 = true
    ipv6 = false
  } : null
}

# cloud-preprod - prepod DS: control-nets 
# There is two different set of subnets in this network
#   - yandex-net-ru-central1-*
#   - ds-ipv6-preprod-*
data "yandex_vpc_network" "preprod_network_control_nets" {
  count      = local.is_prod ? 0 : 1
  network_id = "c6411a5sdbcq67eacggg"
}
data "yandex_vpc_subnet" "preprod_subnets_control_nets" {
  for_each = local.is_prod ? toset([]) : toset(
    data.yandex_vpc_network.preprod_network_control_nets[0].subnet_ids
  )
  subnet_id = each.key
}
locals {
  preprod_control_nets_yandex_ru = local.is_prod ? null : {
    network = data.yandex_vpc_network.preprod_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.preprod_subnets_control_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("yandex-net-ru", subnet.name)) > 0
    }
    routes = []
    ipv4 = true
    ipv6 = true
  }
  preprod_control_nets_ds_ipv6 = local.is_prod ? null : {
    network = data.yandex_vpc_network.preprod_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.preprod_subnets_control_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("ds-ipv6-preprod", subnet.name)) > 0
    }
    routes = flatten([
      for subnet in data.yandex_vpc_subnet.preprod_subnets_control_nets :
      subnet.v6_cidr_blocks
      if length(regexall("ds-ipv6-preprod", subnet.name)) > 0
    ])
    ipv4 = false
    ipv6 = true
  }
}

# cloud-prod - staging DS: control-nets 
# There is two different set of subnets in this network
#   - yandex-net-ru-central1-*
#   - ds-ipv6-preprod-*
data "yandex_vpc_network" "staging_network_control_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enp5s3u3ocfgvk37b8aa"
}
data "yandex_vpc_subnet" "staging_subnets_control_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.staging_network_control_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  staging_control_nets_yandex_ru = local.is_prod ? {
    network = data.yandex_vpc_network.staging_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.staging_subnets_control_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("cloud-data-sphere-preprod-nets-a", subnet.name)) > 0
      # if length(regexall("yandex-net-ru", subnet.name)) > 0

    }
    routes = []
    ipv4 = true
    ipv6 = true
  } : null
  staging_control_nets_ds_ipv6 = local.is_prod ? {
    network = data.yandex_vpc_network.staging_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.staging_subnets_control_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("ds-ipv6-preprod", subnet.name)) > 0
    }
    routes = flatten([
      for subnet in data.yandex_vpc_subnet.preprod_subnets_control_nets :
      subnet.v6_cidr_blocks
      if length(regexall("ds-ipv6-preprod", subnet.name)) > 0
    ])
    ipv4 = false
    ipv6 = true
  } : null
}


# cloud-prod - staging DS: control-nets 
# There is two different set of subnets in this network
#   - yandex-net-ru-central1-*
#   - ds-ipv6-preprod-*
data "yandex_vpc_network" "prod_network_control_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enpbgk3uqo6rh0saou7i"
}
data "yandex_vpc_subnet" "prod_subnets_control_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.prod_network_control_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  prod_control_nets_yandex_ru = local.is_prod ? {
    network = data.yandex_vpc_network.prod_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.prod_subnets_control_nets:
        subnet.zone => subnet.subnet_id
      if length(regexall("cloud-data-sphere-prod-nets-a", subnet.name)) > 0
      # if length(regexall("yandex-net-ru", subnet.name)) > 0
    }
    routes = []
    ipv4 = true
    ipv6 = true
  } : null
  prod_control_nets_ds_ipv6 = local.is_prod ? {
    network = data.yandex_vpc_network.prod_network_control_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.prod_subnets_control_nets :
      subnet.zone => subnet.subnet_id
      if length(regexall("ds-ipv6-prod", subnet.name)) > 0
    }
    routes = flatten([
      for subnet in data.yandex_vpc_subnet.preprod_subnets_control_nets :
      subnet.v6_cidr_blocks
      if length(regexall("ds-ipv6-prod", subnet.name)) > 0
    ])
    ipv4 = false
    ipv6 = true
  } : null
}


# cloud-prod - staging AI: cloud-ml-dev-nets
data "yandex_vpc_network" "staging_network_cloud_ml_dev_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enps564p8bp649d38qgh"
}
data "yandex_vpc_subnet" "staging_subnets_cloud_ml_dev_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.staging_network_cloud_ml_dev_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  staging_cloud_ml_dev_nets = local.is_prod ? {
    network = data.yandex_vpc_network.staging_network_cloud_ml_dev_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.staging_subnets_cloud_ml_dev_nets:
        subnet.zone => subnet.subnet_id
      if length(regexall("cloud-ml-dev-nets-ru", subnet.name)) > 0
    }
    ipv4 = true
    ipv6 = true
  } : null
}

data "yandex_vpc_network" "prod_network_ai_nets" {
  count      = local.is_prod ? 1 : 0
  network_id = "enp53jlg0ing464snc8n"
}
data "yandex_vpc_subnet" "prod_subnets_ai_nets" {
  for_each = local.is_prod ? toset(
    data.yandex_vpc_network.prod_network_ai_nets[0].subnet_ids
  ) : toset([])
  subnet_id = each.key
}
locals {
  prod_ai_nets = local.is_prod ? {
    network = data.yandex_vpc_network.prod_network_ai_nets[0].network_id
    subnets = {
      for subnet in data.yandex_vpc_subnet.prod_subnets_ai_nets :
      subnet.zone => subnet.subnet_id
    }
    ipv4 = true
    ipv6 = true
  } : null
}

locals {
  user_nets = {
    preprod = local.preprod_user_nets
    staging = local.staging_user_nets
    prod    = local.prod_user_nets
  }

  control_nets_yandex_ru = {
    preprod = local.preprod_control_nets_yandex_ru
    staging = local.staging_control_nets_yandex_ru
    prod    = local.prod_control_nets_yandex_ru
  }

  control_nets_ds_ipv6 = {
    preprod = local.preprod_control_nets_ds_ipv6
    staging = local.staging_control_nets_ds_ipv6
    prod    = local.prod_control_nets_ds_ipv6
  }

  cloud_ml_dev_nets = {
    preprod = null
    staging = local.staging_cloud_ml_dev_nets
    prod    = local.prod_ai_nets #TBD
  }
}
