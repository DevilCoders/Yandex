variable "folder_id" {
  type        = string
  description = "The ID of folder to deploy cluster"
}

variable "prefix" {
  type        = string
  description = "Prefix in object names"
}

variable "worker_zones" {
  type        = list(string)
  description = "Zones of K8S workers (each produces an IPv4 and an IPv6 static route)"
}

variable "v6_cidr" {
  type        = string
  description = "Base CIDR of all subnets in this network"
  default     = "fc00::/32"
}

variable "worker_v4_cidr" {
  type        = string
  description = "CIDR block in local network for workers"
}

variable "routable_v4_cidr" {
  type        = string
  description = "CIDR block in remote network routable via instance"
}

variable "worker_v6_cidr" {
  type        = string
  description = "CIDR block in local network for workers"
}

variable "routable_v6_cidr" {
  type        = string
  description = "CIDR block in remote network routable via instance"
}

variable "router_instance_zone" {
  type        = string
  description = "Zone where router instance is located"
}

variable "use_local_compute_node" {
  type        = bool
  description = "Forward traffic through compute-node colocated with agent on the same vrouter"
}

variable "monitoring_network_ipv4_cidrs" {
  type        = map(string)
  description = "IPv4 CIDRs of network for each availability zone"
}

variable "subnet_zone_bits" {
  type        = map(number)
  description = "Mapping from zone id to subnet bits in address"
}
