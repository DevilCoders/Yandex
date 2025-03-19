source qemu ws19core {
  iso_url          = "../iso/windows-server-19.iso"
  iso_checksum     = "sha256:0067afe7fdc4e61f677bd8c35a209082aa917df9c117527fc4b2b52a447e89bb"
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
  sources = ["source.qemu.ws19core"]

  post-processors {
    post-processor manifest {
      output     = "manifest.json"
      strip_path = true
    }
  }
}