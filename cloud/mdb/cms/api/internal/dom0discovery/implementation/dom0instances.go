package implementation

import (
	"context"
	"fmt"

	"github.com/opentracing/opentracing-go"
	tracelog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (d *Dom0DiscoveryImpl) Dom0Instances(ctx context.Context, dom0 string) (dom0discovery.DiscoveryResult, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "Dom0Instances", tags.Dom0Fqdn.Tag(dom0))
	defer span.Finish()
	var result = dom0discovery.DiscoveryResult{
		WellKnown: []models.Instance{},
		Unknown:   []string{},
	}
	dbmContainers, err := d.dbm.Dom0Containers(ctx, dom0)
	if err != nil {
		return result, err
	}
	if len(dbmContainers) == 0 {
		return result, nil
	}

	dbmVolumes, err := d.dbm.VolumesByDom0(ctx, dom0)
	if err != nil {
		return result, err
	}

	for _, dCtr := range dbmContainers {
		fqdn := dCtr.FQDN
		groups, ok := d.cache.HostGroups(fqdn)
		if !ok {
			ctxlog.Error(ctx, d.L, fmt.Sprintf("instance %q is NOT found in conductor cache", fqdn))
			result.Unknown = append(result.Unknown, fqdn)
			continue
		}

		// conductor host ALWAYS has at least one parent group
		if _, ok := d.blackLG[groups[0]]; ok {
			ctxlog.Info(ctx, d.L, fmt.Sprintf("instance %q is blacklisted, skipped", fqdn))
			continue
		}

		ctrVolumes, ok := dbmVolumes[fqdn]
		if !ok {
			ctxlog.Warn(ctx, d.L, fmt.Sprintf("instance %q has no information about its disc volumes", fqdn))
			span.LogFields(tracelog.Event(fmt.Sprintf("instance %q has no information about its disc volumes", fqdn)))
			ctrVolumes = dbm.ContainerVolumes{
				Volumes: []dbm.Volume{},
			}
		}

		isKnown := false
		for _, group := range groups {
			if _, ok := d.whiteLG[group]; ok {
				ctxlog.Info(ctx, d.L, fmt.Sprintf("instance %q is well known", fqdn))
				result.WellKnown = append(result.WellKnown, models.Instance{
					ConductorGroup: groups[0], // parent group
					FQDN:           fqdn,
					DBMClusterName: dCtr.ClusterName,
					Volumes:        ctrVolumes.Volumes,
				})
				isKnown = true
				break
			}
		}

		if !isKnown {
			ctxlog.Error(ctx, d.L, fmt.Sprintf("instance %q is NOT well registered, you should register it properly", fqdn))
			result.Unknown = append(result.Unknown, fqdn)
		}
	}

	return result, nil
}
