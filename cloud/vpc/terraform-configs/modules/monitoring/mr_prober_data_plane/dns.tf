# This DNS zone should be created by "/duty dns" and imported into
# terraform state.
# See https://wiki.yandex-team.ru/cloud/devel/ycdns/welcome/ for details
# 
# Example command for importing:
#   

resource "ycp_dns_dns_zone" "mr_prober" {
  name        = "mr-prober-dns-zone"
  description = "Mr. Prober DNS Zone"

  folder_id   = var.folder_id

  zone        = "${var.dns_zone}."

  public_visibility {
    enable_yandex_dns_sync = true
    yandex_only = true
  }
}
