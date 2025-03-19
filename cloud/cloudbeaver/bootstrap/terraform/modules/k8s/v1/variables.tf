variable "network_id" {
  type        = string
  description = "network for k8s cluster"
}

variable "locations" {
  type = list(object({
    subnet_id = string
    zone      = string
  }))
  description = "list of az for k8s cluster"
}

variable "registry_id" {
  type        = string
  description = "container registry id"
}

variable "folder_id" {
  type        = string
  description = "folder for k8s cluster"
}

variable "docker_registry_pusher" {
  type        = string
  description = "user id for docker push"
}

variable "cluster_name" {
  type        = string
  description = "k8s cluster name"
}

variable "security_groups_ids" {
  type        = list(string)
  description = "list of security groups"
}

variable "service_ipv4_range" {
  type        = string
  description = "non overlapping range for pods, e.g. 10.97.0.0/16"
}

variable "service_ipv6_range" {
  type        = string
  description = "ipv6 range for pods"
}

variable "cluster_ipv4_range" {
  type        = string
  description = "non overlapping range for cluster nodes, e.g. 10.113.0.0/16"
}

variable "cluster_ipv6_range" {
  type        = string
  description = "IPV6 range for cluster nodes"
}

variable "yandex_nets" {
  type = object({
    ipv4 = list(string)
    ipv6 = list(string)
  })
}

variable "healthchecks_cidrs" {
  type = object({
    v4 = list(string)
    v6 = list(string)
  })
}

variable "k8s_node_group" {
  type = object({
    cores         = number
    memory        = number
    core_fraction = number
    size          = number
  })
}
