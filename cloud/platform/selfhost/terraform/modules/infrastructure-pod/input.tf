variable "juggler_check_name" {
  type = string
  default = ""
}

variable "juggler_check_names" {
  type = list(string)
  default = []
}

variable "logcleaner_files_to_keep" {
  type = string
  default = "5"
}

variable "solomon_prometheus_native" {
  default = false
}

variable "solomon_agent_conf_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "push_client_conf_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "juggler_bundle_manifest_path" {
  type = string
  default = "_intentionally_empty_file"
}

variable "platform_http_check_path" {
  type = string
  default = "_intentionally_empty_file"
}
