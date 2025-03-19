source qemu ws16core {
  iso_url          = "../iso/windows-server-16.iso"
  iso_checksum     = "sha256:47919ce8b4993f531ca1fa3f85941f4a72b47ebaa4d3a321fecf83ca9d17e6b8"
  output_directory = "output/"

  accelerator  = "kvm"
  machine_type = "q35"
  qemuargs = [
    ["-parallel", "none"],
    ["-m", "4096M"],
    ["-smp", "cpus=2"],
    ["-nic", "none"]
  ]

  cpus             = 2
  memory           = 4
  disk_size        = "20480M"
  disk_compression = true
  cd_files = [
    "../drivers/0.1.208/netkvm/2k16/amd64/*",
    "../drivers/0.1.208/viostor/2k16/amd64/*",
    "../scripts/qemu/*",
    "Autounattend.xml"
  ]

  communicator     = "none"
  shutdown_timeout = "300m"
}

build {
  sources = ["source.qemu.ws16core"]

  post-processors {
    post-processor manifest {
      output     = "manifest.json"
      strip_path = true
    }
  }
}