variable "cloud_id" {
  type    = string
  default = "yc.yc-s3.serviceCloud"
}

variable "installation" {
  type = object({
    zones = map(object({
      subnet_id = string
    })),
    dns_zone_id            = string
    folder_id              = string
    service_account_prefix = string
    backups_bucket_prefix  = string
    security_groups        = list(string)
    services = object({
      deploy_api          = string
      lockbox             = string
      certificate_manager = string
    })
  })
  default = {
    zones = {
      "il1-a" : {
        subnet_id = "ddkh69d3cmfhh6of0e84"
      }
    }
    zone_id                = "il1-a"
    dns_zone_id            = "yc.yc-s3.dns-zone"
    folder_id              = "yc.database.serviceFolder"
    service_account_prefix = "yc.database"
    backups_bucket_prefix  = ""
    security_groups        = []
    services = {
      deploy_api          = ""
      lockbox             = "dpl.lockbox.private-api.yandexcloud.co.il:8443"
      certificate_manager = "dpl.ycm.private-api.yandexcloud.co.il:8443"
    }
  }
}

variable "image_id" {
  type    = string
  default = "alk0ks20s3f3nsfl7tuj"
}
