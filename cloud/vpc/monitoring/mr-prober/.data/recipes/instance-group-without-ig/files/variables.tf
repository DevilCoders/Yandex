variable "create_folder" {
  type = bool
  description = "Should we create separate folder for this cluster"
  default = false
}

variable "cloud_id" {
  type = string
  description = "In which cloud we should create folder. Can be empty if 'create_folder' is false"
  default = ""
}

variable "prefix" {
  type = string
  description = "Prefix for all objects created in the cluster"
}

variable "vm_count" {
  type = number
  description = "Total amount of virtual machines (agents)"
}

variable "zones" {
  type = list(string)
  description = "List of zones for subnets and virtual machines (agents)"
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "zone_to_dc" {
  type = map(string)
  description = "Mapping from zone (ru-central1-a) to dc (vla) used for instance naming"
  default = {
    ru-central1-a = "vla",
    ru-central1-b = "sas",
    ru-central1-c = "myt"
  }
}

variable "label_environment" {
  type = string
  description = "Environment used for label env in instances"
}

variable "mr_prober_agent_image_name" {
  type = string
  description = "The name of Mr. Prober agent image in https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/"
  default = "fd8a5nv18vv5u40duiig"
}

variable "use_conductor" {
  type = bool
  description = "Should we register entities in conductor? NOTE: conductor is DEPRECATED. Use EDS whenever possible."
  default = false
}

/* VM settings */

variable "add_secondary_disks" {
  type = bool
  description = "Should we create and attach secondary disks to each VM in the cluster"
  default = false
}

variable "vm_platform_id" {
  type = string
  description = "Platform of VMs"
  default = "standard-v2"
}

variable "vm_memory" {
  type = number
  description = "Amount of RAM installed in each VM in the cluster, gigabytes"
  default = 2
}

variable "vm_cores" {
  type = number
  description = "Amount of cores install in each VM in the cluster"
  default = 2
}

variable "vm_core_fraction" {
  type = number
  description = "Core fraction for each VM in the cluster"
  default = 20
}

variable "vm_boot_disk_size" {
  type = number
  description = "Size of boot disk in each VM in the cluster, gigabytes"
  default = 10
}

variable "cloud_init_runcmd" {
  type = list(string)
  description = "List of commands for running on first boot of each VM"
  default = []
}

variable "cloud_init_bootcmd" {
  type = list(string)
  description = "List of commands for running on each boot of each VM"
  default = []
}

/* Network settings */

variable "create_monitoring_network" {
  type = bool
  description = "Should we create separate monitoring network"
  default = false
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
  default = {}
}

variable "monitoring_network_hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on Mr. Prober's monitoring network. Should be false only for private clouds."
  default = true
}

variable "monitoring_network_enable_egress_nat" {
  type = bool
  description = "Should we enable Egress NAT for the monitoring network"
  default = false
}

variable "monitoring_network_add_floating_ip" {
  type = bool
  description = "Should we add FIP for every instance"
  default = false
}