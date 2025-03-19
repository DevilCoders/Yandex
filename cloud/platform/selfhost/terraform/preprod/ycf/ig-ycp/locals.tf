locals {
  template_index         = "{instance.index}"
  template_short_id      = "{instance.short_id}"
  template_internal_dc   = "{instance.internal_dc}"
  template_index_in_zone = "{instance.index_in_zone}"

  environment = "preprod"

  name_prefix = var.conductor_group != "" ? "${var.conductor_group}-${var.name}" : var.name

  dns_prefix  = "${local.name_prefix}-${local.template_internal_dc}${local.template_index_in_zone}"
  shortname   = local.dns_prefix
  nsdomain    = "ycp.cloud-preprod.yandex.net"
  dns_suffix  = ".${local.nsdomain}"
  l3_tg_name  = "${local.name_prefix}-ig-l3-tg"
  l3_req_list = var.l3_tg ? list("qqq") : []
  l7_tg_name  = "${local.name_prefix}-ig-l7-tg"
  l7_req_list = var.l7_tg ? list("qqq") : []
}
