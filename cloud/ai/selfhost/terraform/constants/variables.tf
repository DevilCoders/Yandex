variable "environment" {
  description = "Environment to deploy in"
  type        = string

  // Not supported in tf 0.12
  # validation {
  #   condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
  #   error_message = "The environment value must be one of: preprod, staging, prod."
  # }
}
