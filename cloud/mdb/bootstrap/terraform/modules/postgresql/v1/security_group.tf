resource "yandex_vpc_security_group" "postgres_sg" {
  network_id  = var.network_id
  description = "to connect to postgres"
  dynamic "ingress" {
    for_each = var.connected_security_groups
    content {
      protocol          = "TCP"
      port              = 6432
      description       = "connect from sg to Postgres"
      security_group_id = ingress.value
    }
  }
}

resource "yandex_vpc_security_group" "dns_works" {
  network_id  = var.network_id
  description = "DNS should work, does it not?"
  egress {
    protocol       = "UDP"
    port           = 53
    description    = "dns works"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
  }
}
