# For some stands, this DNS zone should be created by "/duty dns" and imported into
# terraform state.
# See https://wiki.yandex-team.ru/cloud/devel/ycdns/welcome/ for details
# 
# Example command for importing:
#   terragrunt import module.mr_prober.ycp_dns_dns_zone.mr_prober <DNS_ZONE_ID>
#
# For other stands, this DNS zone can be created by terraform, but should be delegated
# by /duty dns. See 
# https://wiki.yandex-team.ru/cloud/devel/ycdns/welcome/#zavedeniezonyvoblachnomdnsdljanovyxokruzhenijjilabovil/gcloud/blue/green/

resource "ycp_dns_dns_zone" "mr_prober" {
  lifecycle {
    prevent_destroy = true
  }

  dns_zone_id = var.dns_zone_id
  name        = "mr-prober-dns-zone"
  description = "Mr. Prober DNS Zone"

  folder_id   = var.folder_id

  zone        = "${var.dns_zone}."

  public_visibility {
    yandex_only = true
    enable_yandex_dns_sync = var.dns_zone_enable_yandex_dns_sync
  }  
}
