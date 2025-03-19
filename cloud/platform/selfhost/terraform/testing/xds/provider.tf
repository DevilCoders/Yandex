provider "ycp" {
  prod      = false
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
  ycp_profile = "testing"
  ycp_config = "../ycp-config.yaml"
}
