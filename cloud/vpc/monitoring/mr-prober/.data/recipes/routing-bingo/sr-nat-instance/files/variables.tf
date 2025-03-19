variable "target_zone_id" {
  type = string
  description = "Zone where ping targets would be placed"
}

variable "external_address_hints" {
  type = list(string)
  description = "Hints passed to VPC-API when creating addresses to use specific FIP Bucket"
}

variable "static_routes" {
  type = list(object({
    destination_prefix = string
    next_hop = string      // "local" or "remote"
  }))
  description = "Definition of static routes in cluster"
}

variable "expected_nexthops" {
  type = list(string)
  description = "Mnemonic for NAT instances picked for each target"
}