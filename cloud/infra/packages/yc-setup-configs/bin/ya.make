OWNER(g:cloud-infra)

PY3_PROGRAM()

PEERDIR(
    cloud/infra/packages/lib
    contrib/python/pyaml
)

PY_SRCS(
    configure_gpu.py
    disk_topology.py
    env_options.py
    huge_pages.py
    isolate_cpu.py
    manage_apt_repos.py
)

END()
