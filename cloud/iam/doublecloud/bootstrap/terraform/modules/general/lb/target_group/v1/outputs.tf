output "target_group_arn" {
  value = aws_lb_target_group.target_group.arn
}

output "target_group_port" {
  value = aws_lb_target_group.target_group.port
}
