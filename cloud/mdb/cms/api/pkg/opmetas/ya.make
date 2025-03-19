GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    cms_instance_whip_primary.go
    dom0_state.go
    ensure_no_primary.go
    lldp.go
    locks_state.go
    metadata.go
    periodic.go
    post_restart.go
    pre_restart.go
    set_downtimes.go
    shipments.go
    stop_containers.go
)

END()
