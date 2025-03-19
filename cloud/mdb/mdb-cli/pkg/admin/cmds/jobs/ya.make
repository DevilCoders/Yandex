GO_LIBRARY()

OWNER(g:mdb)

SRCS(jobs.go)

END()

RECURSE(
    backup-service
    cert-host
    fix-ext-dns
    helpers
    logs
    replace-rootfs
    resetup-host
)
