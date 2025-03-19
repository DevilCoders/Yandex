variable "folder_id" {
  type        = string
  description = "compute folder for VMs and resources"
  default     = "b1gkgl0g8k32577lbdob" // yc-nfs/common
}

variable "service_account_id" {
  type        = string
  description = "service account to perform administrative tasks"
  default     = "ajecknndf75a39o3g09o" // yc-nfs/common/packer-image-builder
}

variable "instance_service_account_id" {
  type        = string
  description = "service account to run VMs"
  default     = "ajejib5pr0a5587d2e37" // yc-nfs/common/filestore-server
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
    vla = "b0c9al94ur3itd157sqk" // yc-nfs/common/cloudvmnets-ru-central1-a
    sas = "e2l4tubebhf1ibrulmrk" // yc-nfs/common/cloudvmnets-ru-central1-b
    myt = "e9bovudo114n8p69n7ka" // yc-nfs/common/cloudvmnets-ru-central1-c
  }
}

variable "dns_zone_id" {
  type    = string
  default = "dns250q1k227h3l8dtd2" // yc.vpc.bootstrap/yc.bootstrap.service-folder/svc.cloud.yandex.net
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
