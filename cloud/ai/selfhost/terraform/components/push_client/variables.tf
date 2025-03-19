variable "environment" {
  description = "Environment to deploy in"
  type        = string

  validation {
    condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
    error_message = "The environment value must be one of: preprod, staging, prod."
  }
}

variable "tag" {
  description = "Container tag from registry"
  type        = string
  default     = null
}

variable "files" {
  description = "List of files to push into logbroker topics"
  type = list(object(
    {
      path  = string
      topic = string
    }
  ))
}

variable "config_file" {
  default = "push-client.yaml"
}