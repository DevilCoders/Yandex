variable "prefix" {
  type = string
  description = "Prefix for all objects created in this cluster"
}

variable "monitoring_network_zones" {
  type = list(string)
  description = "List of zones for subnets"
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "vm_zones" {
  type = list(string)
  description = "List of zones for virtual machines"
  default = ["ru-central1-a", "ru-central1-b"]
}

variable "monitoring_network_ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of network for each availability zone"
  default = {
    ru-central1-a = "10.1.0.0/16"
    ru-central1-b = "10.2.0.0/16"
    ru-central1-c = "10.3.0.0/16"
    il1-a = "10.1.0.0/16"
  }
}

variable "monitoring_network_ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of network for each availability zone"
}

variable "monitoring_network_hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on Mr. Prober's monitoring network. Should be false only for private clouds."
  default = true
}

variable "target_port" {
  type = number
  description = "Target port on backends for balancer"
  default = 80
}

variable "label_environment" {
  type = string
  description = "Environment used for label env in instances"
}

variable "healthcheck_service_source_ipv4_networks" {
  type = list(string)
  description = "List of IPv4 CIDRs of the healthcheck service (https://cloud.yandex.ru/docs/network-load-balancer/concepts/health-check)"
  default = ["198.18.235.0/24", "198.18.248.0/24"]
}

variable "healthcheck_service_source_ipv6_networks" {
  type = list(string)
  description = "List of IPv6 CIDRs of the healthcheck service (https://wiki.yandex-team.ru/cloud/devel/loadbalancing/ipv6/)"
}
