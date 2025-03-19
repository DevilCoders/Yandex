variable "agent_has_sg" {
  type = bool
  description = "Specifies whether SG should be attached to agent (egress) vm"
}

variable "agent_rule" {
  type = object({
    v4_cidr_blocks = list(string),
    v6_cidr_blocks = list(string),
    from_port = number,
    to_port = number,
  })
  description = "Specifies rule that should be set on agent (egress) vm"
}

variable "target_has_sg" {
  type = bool
  description = "Specifies whether SG should be attached to target (ingress) vms"
}

variable "target_rule" {
  type = object({
    v4_cidr_blocks = list(string),
    v6_cidr_blocks = list(string),
    from_port = number,
    to_port = number,
  })
  description = "Specifies rule that should be set on target (ingress) vm"
}

variable "prober_expect" {
  type = string
  description = "Prober expectation: allow (traffic is allowed) or deny (traffic is blocked)"
}
