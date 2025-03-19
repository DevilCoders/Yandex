def check(cluster_map_zones, ids_yaml, filename):
    service_map = {}
    for id_prefix in ids_yaml:
        if id_prefix["location-type"] != "zone":
            continue
        service_map.setdefault(id_prefix["service"], []).append(id_prefix["location"])

    avail_zones = set(cluster_map_zones)
    for service_name, service_zones in service_map.items():
        if not service_zones:
            continue
        missing_zones = avail_zones.difference(service_zones)
        if missing_zones:
            raise Exception("There are missing availability zones {!r} in service {!r} in file {!r}.".format(missing_zones, service_name, filename))

