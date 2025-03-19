// Secrets
variable "folder_id" {
  description = "ID of the folder that the resources belong to"
  type        = string
}

variable "service_account_id" {
  description = "ID of the service account authorized for this instance group"
  type        = string
}

// Target
variable "environment" {
  description = "Environment to deploy in"
  type        = string

  validation {
    condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
    error_message = "The environment value must be one of: preprod, staging, prod."
  }
}

// Instance group description
variable "name" {
  description = "Name of the instance group"
  type        = string
}

variable "description" {
  description = "A description of the instance group"
  type        = string
}

// Resource description
variable "networks" {
  description = "A description of the instance group networks"
  type = list(
    object(
      {
        interface = object(
          {
            network = string
            subnets = map(string)
            ipv4    = bool
            ipv6    = bool
          }
        )
        dns = list(
          object(
            {
              dns_zone_id = string
              fqdn        = string
            }
          )
        )
      }
    )
  )
}

variable "zones" {
  type = list(string)

  # default = [
  #   "ru-central1-a",
  #   "ru-central1-b",
  #   "ru-central1-c",
  # ]
}

variable "instance_group_size" {
  description = "Number of compute instances"
  type        = number
}

variable "resources_per_instance" {
  description = "yandex.cloud.priv.microcosm.instancegroup.v1.ResourcesSpec"
  type = object(
    {
      memory = number
      cores  = number
      gpus   = number
    }
  )

  # TODO: Add validation for parameters
}

variable "boot_disk_spec" {
  description = "Instance boot disc parameters"
  type = object(
    {
      size     = string
      type     = string
      image_id = string
    }
  )
}

variable "secondary_disk_specs" {
  description = "Secondary disk specs"
  default     = []
}

variable "deploy_policy" {
  description = "yandex.cloud.priv.microcosm.instancegroup.v1.DeployPolicy"
  type = object(
    {
      max_unavailable  = number
      max_creating     = number
      max_expansion    = number
      max_deleting     = number
      startup_duration = string
    }
  )
}

// Application config

variable "docker_config" {
  description = "docker auth config"
  type        = string
}

variable "podmanifest" {
  description = "Kubelet pod manifest"
  type        = string
}

variable "configs" {
  description = "configs files"
  type        = string
}

variable "secrets_bundle" {
  description = "configs files"
  type        = string
}

variable "user_data" {
  description = "configs files"
  type        = string
}

variable "additional_metadata" {
  description = "Additional metadata keys (will be mergbed with module-based)"
  type        = map(string)
  default     = {}
}

variable "additional_labels" {
  description = "Additional labels for instance (will be mergbed with module-based)"
  type        = map(string)
  default     = {}
}

// Custom parameters rearly needed to override
variable "platform_id" {
  description = "The ID of the hardware platform configuration for the instance"
  type        = string
  default     = "standard-v2"

  validation {
    condition     = var.platform_id == "standard-v1" || var.platform_id == "standard-v2"
    error_message = "The platform_id value must be one of: standard-v1, standard-v2."
  }
}

variable "scheduling_policy" {
  description = "scheduling policy"
  type        = map(any)
  default = {
    termination_grace_period = "300s"
  }
}

// Legacy parameters for compatibility - Eventually should be removed
variable "target_group_name" {
  default = ""
}

variable "target_group_description" {
  default = ""
}

variable "nlb_target" {
  type = bool
}