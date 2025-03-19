module "application_load_balancers" {
  source = "../modules/alb/v1"

  # id = ds7fdnp449p8r8r9t05q
  name                  = "iam-ya-prestable-l7" # имя самого балансера
  folder_id             = local.iam_ya_prestable_folder.id
  external_ipv6_addresses = [ for a in ycp_vpc_address.iam-ya-prestable-l7.ipv6_address : a.address ]
  certificate_ids       = [
    "fpqsdaeeo0sc288buiph",
  ]
  https_router_id        = ycp_platform_alb_http_router.default.id
  is_edge               = false
  sni_handlers          = []
  allocation            = {
    region_id  = "ru-central1"
    network_id = ycp_vpc_network.cloud-iam-internal-prestable-nets.id
    locations  = [
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-a.id
        zone_id   = "ru-central1-a"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-b.id
        zone_id   = "ru-central1-b"
      },
      {
        subnet_id = ycp_vpc_subnet.cloud-iam-internal-prestable-nets-ru-central1-c.id
        zone_id   = "ru-central1-c"
      },
    ]
  }
  solomon_cluster_name  = "cloud_prod_iam-ya-prestable-router"
  juggler_host          = "cloud_prod_iam-ya-prestable-l7-router"
}
