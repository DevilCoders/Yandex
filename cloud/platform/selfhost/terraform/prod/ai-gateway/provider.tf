provider "yandex" {
    endpoint  = var.yc_endpoint
    token     = var.yc_token
    folder_id = var.yc_folder
    zone      = var.yc_zone

    version = ">= 0.7.0"
}

provider "ycp" {
    prod      = true
    token     = var.yc_token
    folder_id = var.yc_folder
    zone      = var.yc_zone
}
