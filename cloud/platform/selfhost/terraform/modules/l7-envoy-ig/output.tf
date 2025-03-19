output "instance_group_id" {
  value = ycp_microcosm_instance_group_instance_group.this.id
}

output "target_group_id" {
  value = ycp_microcosm_instance_group_instance_group.this.load_balancer_state.0.target_group_id
}
