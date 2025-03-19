data "external" "yav_secret_getter" {
  program = [
    "python3",
    "${path.module}/yav_secret_generator.py"]

  query = {
    yt_oauth   = var.yt_oauth
    id         = var.id
    value_name = var.value_name
  }
}
