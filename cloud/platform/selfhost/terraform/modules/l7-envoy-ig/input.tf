#######################
# Names

variable name {
  type = string # e.g. "cpl"
}

variable conductor_group_suffix {
  type = string # e.g. "cpl"
}

variable ma_name {
  type = string # e.g. "cpl"
}

variable tracing_service {
  type = string # e.g. "cpl-router"
}

variable optional_tg_name {
  type    = string # Name of L3 target group.
  default = null
}

#######################
# IDs

variable alb_lb_id {
  type = string
}

variable image_id {
  type = string
}

variable ig_sa {
  type = string
}

variable instance_sa {
  type = string
}

#######################
# Env

variable env {
  type = string # e.g. "preprod"
}

variable env2 {
  type    = string # e.g. "pre-prod". Differs only for preprod.
  default = null
}

variable domain {
  type = string # e.g. "cloud-testing.yandex.net", for the nsdomain metadata key.
}

#######################
# Resources

variable platform {
  default = "standard-v2"
}

variable memory {
  default = 4
}

variable cores {
  default = 2
}

variable core_fraction {
  default = 100
}

variable boot_disk_size {
  default = 40
}

variable secondary_disk_size {
  default = 40
}

variable size {
  default = 1
}

#######################
# Secrets

variable solomon_token {
  type = string
}

#######################
# Endpoints

variable kms_endpoint {
  type = string # e.g. "kms.cloud-preprod.yandex.net"
}

variable xds_endpoints {
  type = list(string) # e.g. ["2a02:6b8:c0e:2c0:0:fc1a:0:30"]
}

variable xds_sni {
  type = string # e.g. "xds.ycp.cloud-preprod.yandex.net"
}

variable xds_dumps_port {
  default = 18000
}

variable remote_als_addr {
  default = "" # e.g. "als.ycp.cloud-testing.yandex.net"
}

variable als_addr {
  default = "127.0.0.1"
}

variable als_port {
  default = 4436
}

#######################
# Network

variable network_id {
  type = string
}

variable subnet_ids {
  type = list(string)
}

variable zones {
  type = list(string)
}

variable has_ipv4 {
  default = false
}

variable network_type {
  default = "STANDARD"
}

#######################
# Misc.

variable push_client_ident {
  type = string # e.g. "yc_api@preprod"
}

# https://discuss.hashicorp.com/t/tips-howto-implement-module-depends-on-emulation/2305/2
variable "module_depends_on" {
  type    = any
  default = null
}

variable enable_tracing {
  default = true
}

# Override user-data.
variable user_data {
  type    = string
  default = null
}

# Override ssh-keys.
variable ssh_keys {
  type    = string
  default = null
}

# Copy arbitrary files.
variable files {
  type    = map(string)
  default = {}
}

variable k8s_bootstrap {
  default = ""
}

variable xds_auth {
  default = false
}

variable dummy {
  type    = string
  default = null
}

#######################
# Deprecated.

# aka allCAs.pem
variable ca_bundle {
  type    = string
  default = null
}

variable server_cert_pem { # Use "files" instead.
  type    = string
  default = null
}

variable server_cert_key { # Use "files" instead.
  type    = string
  default = null
}

variable client_cert_pem { # Use "files" instead.
  type    = string
  default = null
}

variable client_cert_key { # Use "files" instead.
  type    = string
  default = null
}

variable alb_endpoint {
  default = "not-used.localhost:666"
}

variable cert_manager_endpoint {
  default = "not-used.localhost:666"
}

variable sds_log_level {
  default = "INFO"
}

variable sds_enable_cert_manager {
  default = false
}

###################################
# Legacy stuff.

variable frontend_cert {
  default = true
}

variable xds_client_cert {
  default = true
}