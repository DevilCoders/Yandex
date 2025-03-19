resource "yandex_vpc_security_group" "load_test" {
  folder_id = var.folder_id
  network_id = ycp_vpc_network.network.id
  name = "load-test-sg"
  description = "Security group for load test jump host"

  ingress {
    protocol = "TCP"
    description = "Allow SSH from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "ANY"
    description = "Allow solomon"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    from_port = 10000
    to_port = 10005
  }

  ingress {
    protocol = "ANY"
    description = "Allow inter-group communication"
    predefined_target = "self_security_group"
    from_port = 0
    to_port = 65535
  }

  egress {
    protocol = "ANY"
    description = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port = 0
    to_port = 65535
  }
}