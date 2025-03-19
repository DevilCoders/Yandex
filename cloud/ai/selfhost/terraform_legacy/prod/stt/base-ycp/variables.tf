////////
// Auth data
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "name" {
  description = "Instance group name"
}

// Instance group config
variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = 2
}

variable "max_unavailable" {
  description = "The maximum number of running instances that can be taken offline (stopped or deleted) at the same time during the update process"
  default     = 1
}

variable "max_creating" {
  description = "The maximum number of instances that can be created at the same time"
  default     = 30
}

variable "max_expansion" {
  description = "The maximum number of instances that can be temporarily allocated above the group's target size during the update process"
  default     = 0
}

variable "max_deleting" {
  description = "The maximum number of instances that can be deleted at the same time"
  default     = 5
}

variable "stt_server_version" {
  description = "Version of docker image with stt_server"
}

variable "stt_model_image_id" {
  description = "image id used for creating secondary disk with model"
  default     = ""
}

variable "yc_zones" {
    description = "Yandex Cloud Zones to deploy in"
    default     = ["ru-central1-b", "ru-central1-a"] // SAS, VLA
}

variable "platform_id" {
    type = string
    default = "gpu-standard-v2"
}

variable "instance_gpus" {
    description = "GPUs per one instance"
    default     = "2"
}

variable "instance_cores" {
    description = "Cores per one instance"
    default     = "16"
}

variable "instance_memory" {
    description = "Memory in GB per one instance"
    default     = "96"
}

variable "instance_max_memory" {
    description = "Max memory in GB per one instance"
    default     = "80"
}




