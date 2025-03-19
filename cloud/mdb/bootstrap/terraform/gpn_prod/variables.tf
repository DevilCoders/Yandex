variable "yc_cloud" {
  type    = string
  default = "bn3467a8mq6n6p6o0chf"
}

variable "yc_folder" {
  type    = string
  default = "bn3mr50e0pn51bghu5tq"
}

variable "clusters_folder" {
  type    = string
  default = "bn3qi5stltbat72l70kd"
}

variable "gpn_sa_config" {
  default = ""
}

variable "gpn_sa_file" {
  default = ""
}

locals {
  gpn_sa_file   = (var.gpn_sa_file != "" ? var.gpn_sa_file : pathexpand("~/.config/ycp/gpn-controlplane-sa-key.json"))
  gpn_sa_config = (var.gpn_sa_config != "" ? var.gpn_sa_config : pathexpand("../configs/private-gpn-1.yaml"))
}

variable "ycp_profile" {
  default = "selfhost-profile"
}

variable "zone_id" {
  default = "ru-gpn-spb99"
}

variable "region_id" {
  default = "ru-gpn-spb"
}

variable "zone_suffix" {
  default = "gpn-spb99"
}

variable "fqdn_suffix" {
  default = "gpn.yandexcloud.net"
}

variable "name_format" {
  default = "%s%02g-%s"
}

variable "fqdn_format" {
  default = "%s%02g-%s.%s"
}

variable "s3_admin_access_key" {
  type        = string
  description = "s3-admin access-key"
}

variable "s3_admin_secret_key" {
  type        = string
  description = "s3-admin secret-key"
}

variable "yc_token" {
  type        = string
  description = "Obtain token here https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb"
  default     = null
}

variable "mdb_admin_token" {
  default     = ""
  description = "Token for robot MDB. Use generate-secret-vars.sh with two args"
}

variable "mdb_deploy_api" {
  default = "mdb-deploy.private-api.gpn.yandexcloud.net"
}

variable "s3" {
  type = object({
    cloud_id   = string
    folder_id  = string
    zones      = map(string)
    domain     = string
    deploy_api = string
    cluster = object({
      zk = object({
        shards               = number
        shard_hosts_per_zone = number
        prefix               = string
        platform             = string
        nvme_disks           = number
        cores                = number
        memory_gb            = number
        boot_disk_size_gb    = number
        datadir              = string
      })
      pgmeta = object({
        shards               = number
        shard_hosts_per_zone = number
        prefix               = string
        platform             = string
        nvme_disks           = number
        cores                = number
        memory_gb            = number
        boot_disk_size_gb    = number
        datadir              = string
      })
      s3meta = object({
        shards               = number
        shard_hosts_per_zone = number
        prefix               = string
        platform             = string
        nvme_disks           = number
        cores                = number
        memory_gb            = number
        boot_disk_size_gb    = number
        datadir              = string
      })
      s3db = object({
        shards               = number
        shard_hosts_per_zone = number
        prefix               = string
        platform             = string
        nvme_disks           = number
        cores                = number
        memory_gb            = number
        boot_disk_size_gb    = number
        datadir              = string
      })
    })
  })
  default = {
    cloud_id   = "yc.yc-s3.serviceCloud"
    folder_id  = "yc.database.serviceFolder"
    domain     = "svc.gpn.yandexcloud.net"
    deploy_api = "mdb-deploy.private-api.gpn.yandexcloud.net"
    zones = {
      "ru-gpn-spb99" = "spb99"
    }
    cluster = {
      zk = {
        shards               = 1
        shard_hosts_per_zone = 3
        prefix               = "zk-s3-"
        platform             = "standard-v2"
        nvme_disks           = 0
        cores                = 2
        memory_gb            = 4
        boot_disk_size_gb    = 20
        datadir              = "/var/lib/zookeeper"
      }
      pgmeta = {
        shards               = 1
        shard_hosts_per_zone = 2
        prefix               = "pgmeta"
        platform             = "standard-v2"
        nvme_disks           = 0
        cores                = 2
        memory_gb            = 4
        boot_disk_size_gb    = 30
        datadir              = "/var/lib/postgresql"
      }
      s3meta = {
        shards               = 1
        shard_hosts_per_zone = 2
        prefix               = "s3meta"
        platform             = "standard-v2"
        nvme_disks           = 0
        cores                = 4
        memory_gb            = 16
        boot_disk_size_gb    = 100
        datadir              = "/var/lib/postgresql"
      }
      s3db = {
        shards               = 1
        shard_hosts_per_zone = 2
        prefix               = "s3db"
        platform             = "standard-v2"
        nvme_disks           = 0
        cores                = 4
        memory_gb            = 16
        boot_disk_size_gb    = 200
        datadir              = "/var/lib/postgresql"
      }
    }
  }
}
