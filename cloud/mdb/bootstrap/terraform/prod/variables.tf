variable "cloud_id" {
  type    = string
  default = "b1ggh9onj7ljr7m9cici"
}

variable "folder_id" {
  type    = string
  default = "b1g0r9fh49hee3rsc0aa"
}

variable "region_id" {
  type    = string
  default = "ru-central1"
}

variable "ycp_profile" {
  default = "selfhost-profile"
}

variable "ycp_config" {
  default = ""
}

locals {
  ycp_config = (var.ycp_config != "" ? var.ycp_config : pathexpand("../configs/compute_prod.yaml"))
}

variable "service_account_key_file" {
  type    = string
  default = "./sa.json"
}

variable "s3_admin_access_key" {
  type        = string
  description = "s3-admin access-key"
}

variable "s3_admin_secret_key" {
  type        = string
  description = "s3-admin secret-key"
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
        shards            = number
        prefix            = string
        platform          = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
      pgmeta = object({
        shards            = number
        prefix            = string
        platform          = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
      s3meta = object({
        shards            = number
        prefix            = string
        platform          = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
      s3db_new = object({
        shards            = number
        prefix            = string
        platform          = string
        image             = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
      s3db = object({
        shards            = number
        shards_skip       = number
        prefix            = string
        platform          = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
      s3dbv3 = object({
        shards            = number
        shards_skip       = number
        prefix            = string
        platform          = string
        image             = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
    })
  })
  default = {
    cloud_id   = "b1g5ojh11hk98ou6k241"
    folder_id  = "b1g27hsj71dd60mlgnmc"
    domain     = "svc.cloud.yandex.net"
    deploy_api = "mdb-deploy-api.private-api.yandexcloud.net"
    zones = {
      "ru-central1-a" = "k"
      "ru-central1-b" = "h"
      "ru-central1-c" = "f"
    }
    cluster = {
      zk = {
        shards            = 1
        prefix            = "zk-s3-compute"
        platform          = "standard-v2"
        nvme_disks        = 1
        cores             = 2
        memory_gb         = 8
        boot_disk_size_gb = 20
        datadir           = "/var/lib/zookeeper"
      }
      pgmeta = {
        shards            = 1
        prefix            = "pgmeta"
        platform          = "standard-v2"
        nvme_disks        = 0
        cores             = 2
        memory_gb         = 4
        boot_disk_size_gb = 30
        datadir           = "/var/lib/postgresql"
      }
      s3meta = {
        shards            = 1
        prefix            = "s3meta"
        platform          = "standard-v2"
        nvme_disks        = 3
        cores             = 32
        memory_gb         = 128
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
      s3db_new = {
        shards            = 4
        prefix            = "s3db"
        platform          = "standard-v3"
        image             = "fd882jujhkpb5nfss48c" # ubuntu-1804-lts-1541165525-mlnx-ofed
        nvme_disks        = 4
        cores             = 32
        memory_gb         = 128
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
      s3db = {
        shards            = 12
        shards_skip       = 4
        prefix            = "s3db"
        platform          = "standard-v2"
        nvme_disks        = 13
        cores             = 16
        memory_gb         = 64
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
      s3dbv3 = {
        shards            = 16
        shards_skip       = 16
        prefix            = "s3db"
        platform          = "standard-v3"
        image             = "fd882jujhkpb5nfss48c" # ubuntu-1804-lts-1541165525-mlnx-ofed
        nvme_disks        = 4
        cores             = 32
        memory_gb         = 128
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
    }
  }
}
