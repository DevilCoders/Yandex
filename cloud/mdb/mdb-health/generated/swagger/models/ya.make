GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster_health.go
    cluster_status.go
    cpu_metrics.go
    disk_metrics.go
    error.go
    host_health.go
    host_health_update.go
    host_mode.go
    host_neighbours_info.go
    host_neighbours_resp.go
    host_status.go
    host_system_metrics.go
    hosts_health_resp.go
    hosts_list.go
    memory_metrics.go
    service_health.go
    service_replica_type.go
    service_role.go
    service_status.go
    stats.go
    u_a_availability.go
    u_a_health.go
    u_a_info.go
    u_a_warning_geo.go
)

END()
