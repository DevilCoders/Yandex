source qemu ws12core {
  iso_url          = "../iso/windows-server-12-r2.iso"
  iso_checksum     = "sha256:0e883ce28eb5c6f58a3a3007be978d43edb1035a4585506c1c4504c9e143408d"
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
    "../drivers/0.1.208/netkvm/2k12R2/amd64/*",
    "../drivers/0.1.208/viostor/2k12R2/amd64/*",
    "../scripts/qemu/*",
    "Autounattend.xml"
  ]

  communicator     = "none"
  shutdown_timeout = "300m"
}

build {
  sources = ["source.qemu.ws12core"]

  post-processors {
    post-processor manifest {
      output     = "manifest.json"
      strip_path = true
    }
  }
}