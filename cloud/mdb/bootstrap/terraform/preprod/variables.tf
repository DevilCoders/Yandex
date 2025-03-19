variable "cloud_id" {
  type    = string
  default = "aoe9shbqc2v314v7fp3d"
}

variable "folder_id" {
  type    = string
  default = "aoeme1ci0qvbsjia4ks7"
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
  ycp_config = (var.ycp_config != "" ? var.ycp_config : pathexpand("../configs/compute_preprod.yaml"))
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
      s3db = object({
        shards            = number
        prefix            = string
        platform          = string
        nvme_disks        = number
        cores             = number
        memory_gb         = number
        boot_disk_size_gb = number
        datadir           = string
      })
    })
  })
  default = {
    cloud_id   = "aoehde7k4ac3qs9a7le4"
    folder_id  = "aoe5j0ofetas5ki35gkb"
    domain     = "svc.cloud-preprod.yandex.net"
    deploy_api = "mdb-deploy-api.private-api.cloud-preprod.yandex.net"
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
        memory_gb         = 4
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
        nvme_disks        = 1
        cores             = 8
        memory_gb         = 16
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
      s3db = {
        shards            = 2
        prefix            = "s3db"
        platform          = "standard-v2"
        nvme_disks        = 2
        cores             = 8
        memory_gb         = 16
        boot_disk_size_gb = 20
        datadir           = "/var/lib/postgresql"
      }
    }
  }
}

variable "dataproc_image_test_folder_id" {
  type    = string
  default = "aoe9deomk5fikb8a0f5g"
}

variable "dataproc_infra_test_folder_id" {
  type    = string
  default = "aoeb0d5hocqev4i6rmmf"
}

variable "kubernetes_infra_test_folder_id" {
  type    = string
  default = "aoe78c3212fbi4rpre7n"
}

variable "public_images_folder_id" {
  type    = string
  default = "aoen0gulfkoaicqsvt1i"
}

variable "kubernetes_dataplane_clusters_folder_id" {
  type    = string
  default = "aoekdojbh2aau7lpd5rk"
}
