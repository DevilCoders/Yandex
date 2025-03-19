variable "use_local_compute_node" {
  type        = bool
  description = "Forward traffic through compute-node colocated with agent on the same vrouter"
}

variable "right_zone_id" {
  type        = string
  description = "Name of the zone where right worker node is placed"
}

variable "subnet_zone_bits" {
  type        = map(number)
  description = "Mapping from zone id to subnet bits in address"
  default = {
    ru-central1-a = 0
    ru-central1-b = 1
    ru-central1-c = 2
  }
}
