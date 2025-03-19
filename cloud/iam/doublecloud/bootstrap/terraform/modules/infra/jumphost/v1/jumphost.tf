resource "aws_instance" "instance" {
  ami                    = data.aws_ami.ubuntu.id
  instance_type          = var.instance_type
  subnet_id              = var.subnet_id
  vpc_security_group_ids = var.vpc_security_group_ids

  key_name               = var.ssh_key_name

  tags = {
    jumphost = var.jumphost_name
    Name     = "jumphost ${var.jumphost_name}"
  }

  lifecycle {
    ignore_changes = [
      ami,
    ]
  }
}
