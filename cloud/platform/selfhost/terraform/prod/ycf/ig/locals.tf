locals {
  template_index       = "{instance.index}"
  template_short_id    = "{instance.short_id}"
  template_internal_dc = "{instance.internal_dc}"

  environment = "prod"


  dns_prefix  = "${var.name}-${local.environment}-${local.template_internal_dc}${local.template_index}"
  dns_suffix  = ".ycp.cloud.yandex.net"
  lb_tg_name  = "${var.name}-ig-lb-tg"
  lb_req_list = var.l3_tg ? list(var.l3_tg[0]) : []
}