variable "ami" {
  default = "foo"
  type    = string
}

variable "list" {
  default = []
  type    = list
}

variable "map" {
  default = {}
  type    = map
}

resource "aws_instance" "bar" {
  ami = "${var.ami}"
  instance_type = "${join(",", var.list)}"
  tags = "${var.map}"
}
