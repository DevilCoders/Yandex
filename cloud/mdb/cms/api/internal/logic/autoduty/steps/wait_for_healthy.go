package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/dustin/go-humanize"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
)

type WaitAllHealthyStep struct {
	dom0d   dom0discovery.Dom0Discovery
	health  client.MDBHealthClient
	juggler healthiness.Healthiness
	rr      healthdbspec.RoleSpecificResolvers
}

func (s *WaitAllHealthyStep) GetStepName() string {
	return "health of clusters"
}

func containerToString(cntr models.Instance) string {
	return fmt.Sprintf("'%s' of '%s'", cntr.FQDN, cntr.DBMClusterName)
}

func HealthToString(fc healthiness.FQDNCheck) string {
	ha := false
	haSpecific := ""
	if fc.HACluster || fc.HAShard {
		ha = true
		var haRoles []string
		if fc.HACluster {
			haRoles = append(haRoles, "cluster")
		}
		if fc.HAShard {
			haRoles = append(haRoles, "shard")
		}
		haSpecific = fmt.Sprintf(" (%s)", strings.Join(haRoles, "+"))
	}
	rolesPlayed := "UNKNOWN"
	if len(fc.Roles) > 0 {
		rolesPlayed = strings.Join(fc.Roles, ",")
	}
	hint := ""
	if fc.HintIs != "" {
		hint = fc.HintIs + " "
	}
	return fmt.Sprintf(
		"%s%s HA: %v%s, roles: %s, giving away this node will leave %d healthy nodes of %d total, space limit %s",
		hint,
		containerToString(fc.Instance),
		ha,
		haSpecific,
		rolesPlayed,
		fc.CntAliveLeftInGroup,
		fc.CntTotalInGroup,
		humanize.Bytes(uint64(fc.Instance.VolumesTotalSpaceLimit())),
	)
}

func FCToStrings(fcs []healthiness.FQDNCheck) []string {
	r := make([]string, len(fcs))
	for index, fc := range fcs {
		r[index] = HealthToString(fc)
	}
	return r
}

func FCToContainers(fcs []healthiness.FQDNCheck) []string {
	r := make([]string, len(fcs))
	for index, fc := range fcs {
		r[index] = fmt.Sprintf("%s, reason: %q", containerToString(fc.Instance), fc.HintIs)
	}
	return r
}

func fmtHealthCheck(c healthiness.HealthCheckResult) string {
	resultLines := []string{}
	if len(c.Stale) > 0 {
		resultLines = append(resultLines, fmt.Sprintf("outdated info about %d:\n%s",
			len(c.Stale),
			strings.Join(FCToStrings(c.Stale), "\n")))
	}
	if len(c.Unknown) > 0 {
		resultLines = append(resultLines, fmt.Sprintf("knows nothing about %d:\n%s",
			len(c.Unknown),
			strings.Join(FCToContainers(c.Unknown), "\n")))
	}
	if len(c.WouldDegrade) > 0 {
		resultLines = append(resultLines, fmt.Sprintf("%d would degrade now:\n%s",
			len(c.WouldDegrade),
			strings.Join(FCToStrings(c.WouldDegrade), "\n")))
	}
	if len(c.Ignored) > 0 {
		resultLines = append(resultLines, fmt.Sprintf("and %d looking suspicious but I will ignore them:\n%s",
			len(c.Ignored),
			strings.Join(FCToStrings(c.Ignored), "\n")))
	}
	if len(resultLines) == 0 {
		resultLines = append(resultLines, fmt.Sprintf("%d ok", len(c.GiveAway)))
	}
	return strings.Join(resultLines, "\n")
}

func mergeSingleFieldHealthInfo(primary, secondary []healthiness.FQDNCheck) []healthiness.FQDNCheck {
	for _, check := range secondary {
		found := false
		for _, candidate := range primary {
			if candidate.Instance.FQDN == check.Instance.FQDN {
				found = true
				break
			}
		}
		if !found {
			primary = append(primary, check)
		}
	}
	return primary
}

func OverrideResult(source, overrides healthiness.HealthCheckResult) healthiness.HealthCheckResult {
	return healthiness.HealthCheckResult{
		Unknown:      overrides.Unknown,
		Stale:        mergeSingleFieldHealthInfo(source.Stale, overrides.Stale),
		WouldDegrade: mergeSingleFieldHealthInfo(source.WouldDegrade, overrides.WouldDegrade),
		Ignored:      mergeSingleFieldHealthInfo(source.Ignored, overrides.Ignored),
		GiveAway:     mergeSingleFieldHealthInfo(source.GiveAway, overrides.GiveAway),
	}
}

func (s *WaitAllHealthyStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	mdbHealth := healthbased.NewHealthBasedHealthiness(s.health, s.rr, time.Now())
	rd := execCtx.GetActualRD()
	cntrs, err := s.dom0d.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return escalateWithErrAndMsg(err, "could not get list of containers on dom0")
	}
	health, err := mdbHealth.ByInstances(ctx, cntrs.WellKnown)
	if err != nil {
		return escalateWithErrAndMsg(err, "could not get info from MDB Health")
	}
	if len(health.Unknown) > 0 {
		unknown := make([]models.Instance, len(health.Unknown))
		for ind, u := range health.Unknown {
			unknown[ind] = u.Instance
		}
		heathFromJuggler, err := s.juggler.ByInstances(ctx, unknown)
		if err != nil {
			return escalateWithErrAndMsg(err, "could not get info from Juggler")
		}
		health = OverrideResult(health, heathFromJuggler)
	}

	msg := fmtHealthCheck(health)

	if len(health.Stale) > 0 {
		return waitWithMessage(msg)
	}
	if len(health.Unknown) == 0 && len(health.WouldDegrade) == 0 {
		return continueWithMessage(msg)
	}
	return escalateWithMessage(msg)
}

func NewWaitAllHealthyStep(dom0d dom0discovery.Dom0Discovery, health client.MDBHealthClient, juggler healthiness.Healthiness) DecisionStep {
	return &WaitAllHealthyStep{
		dom0d:   dom0d,
		health:  health,
		rr:      healthdbspec.NewRoleSpecificResolver(),
		juggler: juggler,
	}
}
