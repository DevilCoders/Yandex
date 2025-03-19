package clickhouse

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
)

const (
	planMaxSpeed = 10
)

func Planner(cluster planner.Cluster) ([][]string, error) {
	if len(cluster.Hosts) == 0 {
		return nil, fmt.Errorf("got empty cluster: %+v", cluster)
	}

	orderedHostnames := getHostnames(cluster)
	sort.Strings(orderedHostnames)

	subclusterGroups, err := groupBySubclusters(orderedHostnames, cluster)
	if err != nil {
		return nil, err
	}

	var plan [][]string
	for _, subclusterGroup := range subclusterGroups {
		geoGroups, err := groupByGeo(subclusterGroup, cluster)
		if err != nil {
			return nil, err
		}
		for _, geoGroup := range geoGroups {
			plan = append(plan, planner.LinearPlan(geoGroup, planMaxSpeed)...)
		}
	}
	return plan, nil
}

func getHostnames(cluster planner.Cluster) []string {
	fqdns := make([]string, 0, len(cluster.Hosts))
	for fqdn := range cluster.Hosts {
		fqdns = append(fqdns, fqdn)
	}
	return fqdns
}

func groupBySubclusters(fqdns []string, cluster planner.Cluster) ([][]string, error) {
	subclusterGroups := make(map[string][]string)
	for _, fqdn := range fqdns {
		host, ok := cluster.Hosts[fqdn]
		if !ok {
			return nil, fmt.Errorf("cannot find host %q in cluster %+v", fqdn, cluster)
		}
		subcid := host.Tags.Meta.SubClusterID
		subclusterGroup := subclusterGroups[subcid]
		subclusterGroup = append(subclusterGroup, fqdn)
		subclusterGroups[subcid] = subclusterGroup
	}

	// sort keys for stable result order
	subcids := make([]string, 0, len(subclusterGroups))
	for subcid := range subclusterGroups {
		subcids = append(subcids, subcid)
	}
	sort.Strings(subcids)

	res := make([][]string, 0, len(subclusterGroups))
	for _, subcid := range subcids {
		res = append(res, subclusterGroups[subcid])
	}
	return res, nil
}

func groupByGeo(fqdns []string, cluster planner.Cluster) ([][]string, error) {
	geoGroups := make(map[string][]string)
	for _, fqdn := range fqdns {
		host, ok := cluster.Hosts[fqdn]
		if !ok {
			return nil, fmt.Errorf("cannot find host %q in cluster %+v", fqdn, cluster)
		}
		geo := host.Tags.Geo
		geoGroup := geoGroups[geo]
		geoGroup = append(geoGroup, fqdn)
		geoGroups[geo] = geoGroup
	}

	// sort keys for stable result order
	geos := make([]string, 0, len(geoGroups))
	for geo := range geoGroups {
		geos = append(geos, geo)
	}
	sort.Strings(geos)

	res := make([][]string, 0, len(geoGroups))
	for _, geo := range geos {
		res = append(res, geoGroups[geo])
	}
	return res, nil
}
