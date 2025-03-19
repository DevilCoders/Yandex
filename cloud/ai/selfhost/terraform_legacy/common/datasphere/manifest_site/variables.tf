////////
// Auth data
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

// General
variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-b"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
}

variable "yc_sa_id" {
  description = "ID of the service account authorized for this instance"
}


// Instance group parameters
variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "name" {
  description = "Instance group name"
}

variable "environment" {
  description = "Environment name"
}

// Deployment policy
variable "max_unavailable" {
  default     = 3
  description = "The maximum number of running instances that can be taken offline (stopped or deleted) at the same time during the update process"
}
variable "max_creating" {
  default     = 30
  description = "The maximum number of instances that can be created at the same time"
}
variable "max_expansion" {
  default     = 0
  description = "The maximum number of instances that can be temporarily allocated above the group's target size during the update process"
}
variable "max_deleting" {
  default     = 4
  description = "The maximum number of instances that can be deleted at the same time"
}

variable "startup_duration" {
  default     = "60s" // After usual restart it still wait for this time
  description = "The amount of time in seconds to allow for an instance to start. Instance will be considered up and running (and start receiving traffic) only after the startup_duration has elapsed and all health checks are passed"
}

// Instance parameters
variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "8"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "128"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "image_id" {
  description = "The disk image to initialize this disk from"
  default     = "fd80sfc7mamrj2v2gre8"
}

variable "healthcheck_service_name" {
  description = "Healthcheck service name"
  default     = "manifest-site"
}

// App docker image versions
variable "app_image" {
  description = "Image of main app"
  default     = "node-deployer-manifest-site"
}

variable "app_version" {
  description = "Version of main app"
  default     = "0.8"
}

variable "cadvisor_version" {
  description = "Version of docker image with cadvisor. Look at https://hub.docker.com/r/google/cadvisor/tags"
  default     = "v0.32.0"
}

//Infrastructure image versions
variable "metadata_version" {
  type    = string
  default = "1b931b4a1"
}

variable "solomon_agent_version" {
  type    = string
  default = "7095005"
}

variable "push_client_version" {
  type    = string
  default = "7535166"
}

variable "logrotate_version" {
  type    = string
  default = "0.1"
}

variable custom_env_vars {
  type    = list(any)
  default = []
}
