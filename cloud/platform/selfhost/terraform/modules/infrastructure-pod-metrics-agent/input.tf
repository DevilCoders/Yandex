variable "juggler_check_name" {
  type    = string
  default = ""
}

variable "juggler_check_names" {
  type    = list(string)
  default = []
}

variable "logcleaner_files_to_keep" {
  type    = string
  default = "5"
}

variable "push_client_conf_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "metrics_agent_conf_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "metrics_agent_memory_limit" {
  type    = string
  default = "128Mi"
}

variable metrics_agent_vars {
  type    = map(string)
  default = {}
}

variable "juggler_bundle_manifest_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "platform_http_check_path" {
  type    = string
  default = "_intentionally_empty_file"
}

variable "solomon_shard_cluster" {
  type    = string
  default = ""
}

variable "is_gateway" {
  default = false
}

variable "is_xds" {
  default = false
}

variable "is_mk8s_master" {
  default = false
}
