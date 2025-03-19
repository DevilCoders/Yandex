module "application_load_balancers" {
  source = "../modules/alb/v1"

  name                  = "auth-israel-l7"
  folder_id             = local.openid_folder.id
  external_ipv6_addresses = [ for a in ycp_vpc_address.auth_l7_external_ipv6.ipv6_address : a.address ]
  external_ipv4_addresses = [ for a in ycp_vpc_address.auth_l7_external_ipv4.external_ipv4_address : a.address ]
  certificate_ids       = [
    ycp_certificatemanager_certificate_request.auth.id
  ]
  https_router_id       = ycp_platform_alb_http_router.auth["https"].id
  http_router_id        = ycp_platform_alb_http_router.auth["http"].id
  is_edge               = false
  sni_handlers          = []
  allocation            = {
    region_id  = local.region_id
    network_id = local.vpc.auth.network.id
    locations  = [
      for _, subnet in local.vpc.auth.subnets : {
        subnet_id = subnet.id
        zone_id   = subnet.zone_id
      }
    ]
  }
  solomon_cluster_name  = "cloud_prod_auth-israel-l7-router"
  juggler_host          = "cloud_prod_auth-israel-l7-router"
}
