// Secrets
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
  type        = string
  sensitive   = true
}

variable "service_account_key_file" {
  description = "Yandex Team security OAuth token"
  type        = string
  sensitive   = true
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

variable "folder_id" {
  description = "Yandex Cloud Folder ID where resources will be created"
  type        = string
  # TODO: Add validation block
}

variable "service_account_id" {
  description = "ID of the service account authorized for this instance"
  type        = string
  # TODO: Add validation block
}

variable "instance_group_size" {
  type = number
}