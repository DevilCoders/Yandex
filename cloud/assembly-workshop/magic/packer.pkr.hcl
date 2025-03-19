source "file" "basic-example" {
  content =  "Lorem ipsum dolor sit amet"
  target =  "dummy_artifact"
}

build {
  sources = ["sources.file.basic-example"]
}
