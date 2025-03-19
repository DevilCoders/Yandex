variable "folder_id" {
  type        = string
  description = "The ID of folder to deploy Mr. Prober"
  // Well-known id of "mr-prober" folder in "yc.vpc.monitoring" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  default     = "yc.vpc.mr-prober"
}

variable "yc_profile" {
  type        = string
  description = "Name of the yc profile"
}

variable "yc_endpoint" {
  type        = string
  description = "Endpoint for public API (for yandex provider)"
}

variable "prefix" {
  type        = string
  description = "Prefix for all objects VMs created by this module"
}

variable "control_network_id" {
  type        = string
  description = "The id of Mr. Prober control network"
}

variable "control_network_subnet_ids" {
  type        = map(string)
  description = "Ids of Mr. Prober control network subnets for each availability zone"
}

variable "mr_prober_sa_id" {
  type        = string
  description = "The ID of Mr. Prober service account mr-prober-sa"
}

variable "mr_prober_secret_kek_id" {
  type        = string
  description = "The ID of Mr. Prober key for encrypting SKM metadata"
}

variable "dns_zone" {
  type        = string
  description = "DNS zone for all VMs in cluster. Should be equal to YTR settings. I.e. prober.cloud.yandex.net"
}

variable "dns_zone_id" {
  type        = string
  description = "The ID of Mr. Prober DNS zone (defined in `dns_zone`)"
}


variable "zones" {
  type        = list(string)
  description = "List of zones for network subnets, instance group and internal load balancer."
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "network_ipv4_cidrs" {
  type        = map(string)
  description = "IPv4 CIDRs of network for each availability zone"
  default     = {
    ru-central1-a = "10.1.0.0/16"
    ru-central1-b = "10.2.0.0/16"
    ru-central1-c = "10.3.0.0/16"
  }
}

variable "label_environment" {
  type        = string
  description = "Environment used for label env in instances"
}

variable "compute_nodes" {
  type        = list(string)
  description = "List of compute nodes for instances creating"
}

variable "cluster_id" {
  type        = number
  description = "Mr. Prober cluster id"
}

variable "mr_prober_conductor_group_id" {
  type        = number
  description = "The ID of conductor group for all Mr. Prober clusters"
}
