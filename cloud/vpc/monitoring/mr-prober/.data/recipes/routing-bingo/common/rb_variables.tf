variable "prefix" {
  type = string
  description = "Prefix for all objects created in this cluster"
}

variable "monitoring_network_zones" {
  type = list(string)
  description = "List of zones for subnets"
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "compute_platform_id" {
  type = string
  description = "Compute platform for VMs"
  default = "standard-v2"
}

variable "compute_nodes" {
  type = list(object({
    zone = string
    fqdn = string
    has_agent = bool
  }))
  description = "Compute node lists to place virtual machines."
}

variable "monitoring_network_ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of network for each availability zone"
  default = {
    ru-central1-a = "10.1.0.0/16"
    ru-central1-b = "10.2.0.0/16"
    ru-central1-c = "10.3.0.0/16"
  }
}

variable "monitoring_network_ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of network for each availability zone"
}

variable "label_environment" {
  type = string
  description = "Environment used for label env in instances"
}
