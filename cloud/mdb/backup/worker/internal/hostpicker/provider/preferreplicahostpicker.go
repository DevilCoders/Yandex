package hostpicker

import (
	"context"
	"strconv"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	healthclient "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type PriorityArgs struct {
	PillarPath []string
}

type PreferReplicaHostPicker struct {
	health      healthclient.MDBHealthClient
	mdb         metadb.MetaDB
	lg          log.Logger
	healthRetry *retry.BackOff
	cfg         Config
	serviceName string
	args        PriorityArgs
}

type hostPriority struct {
	fqdn     string
	priority int
}

func NewPreferReplicaHostPicker(mdb metadb.MetaDB, health healthclient.MDBHealthClient, lg log.Logger, cfg Config, serviceName string, args PriorityArgs) *PreferReplicaHostPicker {
	return &PreferReplicaHostPicker{
		mdb:    mdb,
		health: health,
		lg:     lg,
		healthRetry: retry.New(retry.Config{
			MaxRetries: cfg.Config.HealthMaxRetries,
		}),
		cfg:         cfg,
		serviceName: serviceName,
		args:        args,
	}
}

func (hp *PreferReplicaHostPicker) PickHost(ctx context.Context, fqdns []string) (string, error) {
	healths, err := hp.health.GetHostsHealth(ctx, fqdns)
	if err != nil {
		return "", err
	}

	var parsedPriorities map[string]int
	if hp.args.PillarPath != nil {
		rawPriorities, err := hp.mdb.HostsPillarByPath(ctx, fqdns, hp.args.PillarPath)
		if err != nil {
			return "", err
		}

		parsedPriorities, err = parseIntegerPriorities(rawPriorities)
		if err != nil {
			return "", err
		}
	}

	rolesHosts := getMostSuitableBackupForRoles(healths, fqdns, parsedPriorities, hp.serviceName)
	for _, status := range []types.ServiceRole{types.ServiceRoleReplica, types.ServiceRoleMaster} {
		if host, ok := rolesHosts[status]; ok {
			return host.fqdn, nil
		}
	}
	return "", xerrors.Errorf("failed to pick suitable host from: %+v", healths)
}

func getMostSuitableBackupForRoles(healths []types.HostHealth, fqdns []string, priorities map[string]int, servName string) map[types.ServiceRole]hostPriority {
	fqdnSet := make(map[string]bool)
	for _, f := range fqdns {
		fqdnSet[f] = true
	}

	roles := make(map[types.ServiceRole]hostPriority)

	var replaceHost func(fqdn string, role types.ServiceRole) (newHost hostPriority, needReplace bool)

	if priorities != nil {
		replaceHost = func(fqdn string, role types.ServiceRole) (newHost hostPriority, needReplace bool) {
			if currRoleHost, ok := roles[role]; ok {
				// compare priorities
				newPriority := getPriorityOrDefault(priorities, fqdn)
				if currRoleHost.priority < newPriority {
					return hostPriority{fqdn: fqdn, priority: getPriorityOrDefault(priorities, fqdn)}, true
				}
			} else {
				// Just add host with role
				return hostPriority{fqdn: fqdn, priority: getPriorityOrDefault(priorities, fqdn)}, true
			}

			return hostPriority{}, false
		}
	} else {
		replaceHost = func(fqdn string, role types.ServiceRole) (newHost hostPriority, needReplace bool) {
			if _, ok := roles[role]; !ok {
				// Just add host with role
				return hostPriority{fqdn: fqdn, priority: getPriorityOrDefault(priorities, fqdn)}, true
			}

			return hostPriority{}, false
		}
	}

	for _, health := range healths {
		for _, s := range health.Services() {
			if s.Name() != servName {
				continue
			}
			if s.Status() != types.ServiceStatusAlive {
				continue
			}

			if _, ok := fqdnSet[health.FQDN()]; ok {
				if newHost, replace := replaceHost(health.FQDN(), s.Role()); replace {
					roles[s.Role()] = newHost
				}
			}

		}
	}
	return roles
}

func getPriorityOrDefault(priorities map[string]int, fqdn string) int {
	if p, ok := priorities[fqdn]; ok {
		return p
	}

	return 0
}

func parseIntegerPriorities(rawPriorities map[string]string) (map[string]int, error) {
	result := make(map[string]int)

	for k, v := range rawPriorities {
		if i, err := strconv.Atoi(v); err != nil {
			return nil, err
		} else {
			result[k] = i
		}
	}

	return result, nil
}
