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

variable "additional_files" {
  description = "Additional files to rotate"
  type        = list(string)
  default     = []
}