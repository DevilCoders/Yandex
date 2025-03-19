variable "env" {
  type        = string
  description = "environment name (prod, preprod, testing)"
}

variable "endpoint" {
  type        = string
  description = "API endpoint to use"
}

variable "token" {
  type        = string
  sensitive   = true
  description = "IAM token for authentication (leave empty to use service account inside VM)"
}

variable "bastion_host" {
  type        = string
  description = "bastion host for SSH access"
}

variable "bastion_agent_auth" {
  type        = bool
  description = "turn on bastion authentication"
}

variable "folder_id" {
  type        = string
  description = "compute folder for VMs and resources"
}

variable "zone_id" {
  type        = string
  description = "compute zone for VMs"
}

variable "subnet_id" {
  type        = string
  description = "vpc subnet for VMs"
}

variable "docker_registry" {
  type        = string
  description = "docker registry to pull images from"
}

variable "source_image_id" {
  type        = string
  description = "compute image to use as source template"
  default     = "fd8q0l0a5nlmh4rjhh72" // yc-ycloud-deploy/paas-images/paas-base-g4-20210817102651
}

variable "service_account_id" {
  type        = string
  description = "service account for VMs"
}

variable "app_version" {
  type        = string
  description = "application version to build"
}

variable "image_version" {
  type        = string
  description = "image version to build"
}

variable "user" {
  type    = string
  default = env("USER")
}

variable "bucket_id" {
  type        = string
  description = "S3 bucket to store result images"
}
