source qemu w10ent {
  iso_url          = "../iso/windows-10-21h2.iso"
  iso_checksum     = "sha256:9e678a7f5663df5f041d369bdcfc14ed4f7188abf1d44b6ebbc220ab42ebf2c7"
  output_directory = "output/"

  accelerator  = "kvm"
  machine_type = "q35"
  qemuargs = [
    [ "-parallel", "none" ],
    [ "-m", "4096M" ],
    [ "-smp", "cpus=2" ],
    [ "-nic", "none" ]
  ]

  cpus             = 2
  memory           = 4
  disk_size        = "20480M"
  disk_compression = true
  cd_files = [
    "../drivers/0.1.208/w10/*",
    "../scripts/qemu/*",
    "../scripts/unattend/SetupWinRM.ps1",
    "Autounattend.xml"
  ]

  communicator     = "none"
  shutdown_timeout = "300m"
}

build {
  sources = ["source.qemu.w10ent"]

  post-processors {
    post-processor manifest {
      output     = "manifest.json"
      strip_path = true
    }
  }
}
