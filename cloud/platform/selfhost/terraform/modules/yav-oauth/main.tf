data "external" "code" {
  program = [
    "${path.module}/code.sh"]
}


output "result" {
  value = "${data.external.code.result["oauth"]}"
}
