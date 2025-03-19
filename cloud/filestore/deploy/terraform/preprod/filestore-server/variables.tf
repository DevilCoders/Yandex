variable "folder_id" {
  type        = string
  description = "compute folder for VMs and resources"
  default     = "aoegq6ualqc2jhlo6n55" // yc-nfs/common
}

variable "service_account_id" {
  type        = string
  description = "service account to perform administrative tasks"
  default     = "bfbopssq6pb0gnu9lhes" // yc-nfs/common/packer-image-builder
}

variable "instance_service_account_id" {
  type        = string
  description = "service account to run VMs"
  default     = "bfb1mo4cbf3skfmr44uo" // yc-nfs/common/filestore-server
}

variable "zone_ids" {
  type = map(string)
  default = {
    vla = "ru-central1-a"
    sas = "ru-central1-b"
    myt = "ru-central1-c"
  }
}

variable "subnet_ids" {
  type = map(string)
  default = {
    vla = "buch8k91u9obuj649b0a" // yc-nfs/common/preprod-cloudvmnets-ru-central1-a
    sas = "blt1qofpomgoi0pe85ek" // yc-nfs/common/preprod-cloudvmnets-ru-central1-b
    myt = "fo2tfjkt9mqras6cvbbb" // yc-nfs/common/preprod-cloudvmnets-ru-central1-c
  }
}

variable "dns_zone_id" {
  type    = string
  default = "aet0p7kuafjanarqapj3" // yc.vpc.bootstrap/yc.bootstrap.service-folder/svc.cloud-preprod.yandex.net
}

variable "zone" {
  type        = string
  description = "zone to deploy to"
  default     = ""
}

variable "image_version" {
  type        = string
  description = "image version to deploy"
  default     = ""
}
