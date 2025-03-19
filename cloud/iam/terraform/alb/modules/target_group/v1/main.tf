resource "ycp_platform_alb_target_group" "target_group" {
  name        = var.name
  folder_id   = var.folder_id
  description = var.description
  labels      = {}

  dynamic "target" {
    for_each = var.target_addresses
    content {
      ip_address = target.value
    }
  }
}
