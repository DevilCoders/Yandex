source qemu ws22core {
  iso_url          = "../iso/windows-server-22.iso"
  iso_checksum     = "sha256:611ffcd6d08e9a6bb8b30fa7df787d96e368b852e3c3000795f6a2b5f07e0a9d"
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
    "../drivers/0.1.208/netkvm/2k19/amd64/*",
    "../drivers/0.1.208/viostor/2k19/amd64/*",
    "../scripts/qemu/*",
    "Autounattend.xml"
  ]

  communicator     = "none"
  shutdown_timeout = "300m"
}

build {
  sources = ["source.qemu.ws22core"]

  post-processors {
    post-processor manifest {
      output     = "manifest.json"
      strip_path = true
    }
  }
}