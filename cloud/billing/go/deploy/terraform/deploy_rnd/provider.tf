provider "yandex" {
  endpoint  = var.yc_endpoint
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

provider "ycp" {
  prod      = true
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

provider "ytr" {
  juggler_token = var.yt_token
}
