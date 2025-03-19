package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

type DrillConfig struct {
	From       encodingutil.DateTime `json:"from" yaml:"from"`
	Till       encodingutil.DateTime `json:"till" yaml:"till"`
	GeoNames   GeoNames              `json:"availability_zones" yaml:"availability_zones"`
	Ticket     string                `json:"ticket" yaml:"ticket"`
	WaitBefore encodingutil.Duration `json:"wait_before" yaml:"wait_before"`
	WaitAfter  encodingutil.Duration `json:"wait_after" yaml:"wait_after"`
}

type DrillsConfig []DrillConfig

type GeoNames []string

type CheckDrills struct {
	now  time.Time
	cfg  DrillsConfig
	mdb  metadb.MetaDB
	dom0 dom0discovery.Dom0Discovery
	cncl conductor.Client
}

type GeoMatchResult struct {
	matched  []string
	notfound []string
}

func (g GeoNames) matchGeo(hostGeo string) bool {
	for _, geo := range g {
		if hostGeo == geo {
			return true
		}
	}
	return false
}

func (s *CheckDrills) GetStepName() string {
	return "check next drills"
}

func (s *CheckDrills) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	willDrillSoon := false
	result := newGeoMatchResult()
	for _, d := range s.cfg {
		if ok, drillsMsg := matchDates(s.now, d); ok {
			willDrillSoon = true
			dom0FQDN := execCtx.GetActualRD().R.MustOneFQDN()
			is, err := s.dom0.Dom0Instances(ctx, dom0FQDN)
			if err != nil {
				return escalateWithErrAndMsg(err, fmt.Sprintf("failed to get dom0 containers '%s'", dom0FQDN))
			}

			drillResult, err := matchGeo(ctx, s.mdb, is.WellKnown, d, s.cncl)
			if err != nil {
				return escalateWithErrAndMsg(err, "Can not check geo")
			}

			if len(drillResult.matched) > 0 {
				msg := fmt.Sprintf("%s. There are %d legs in those locations affect this request.\n* %s",
					drillsMsg,
					len(drillResult.matched),
					strings.Join(drillResult.matched, "\n* "),
				)
				return escalateWithMessage(formatResultMessage(msg, drillResult.notfound))
			}
			result.notfound = append(result.notfound, drillResult.notfound...)
		}
	}
	if willDrillSoon {
		return continueWithMessage(formatResultMessage("drills found, but no problems for this event expected", result.notfound))
	}
	return continueWithMessage("there are no drills")
}

func matchGeo(ctx context.Context, mdb metadb.MetaDB, is []models.Instance, d DrillConfig, cncl conductor.Client) (GeoMatchResult, error) {
	result := newGeoMatchResult()
	mctx, err := mdb.Begin(ctx, sqlutil.PreferStandby)
	if err != nil {
		return result, err
	}
	defer func() { _ = mdb.Rollback(mctx) }()

	for _, instance := range is {
		host, err := mdb.GetHostByFQDN(mctx, instance.FQDN)
		if err == metadb.ErrDataNotFound {
			cndResult, err := matchGeoConductorHost(ctx, cncl, instance.FQDN, d)
			if err != nil {
				return cndResult, err
			}
			result.matched = append(result.matched, cndResult.matched...)
			result.notfound = append(result.notfound, cndResult.notfound...)
		} else {
			if err != nil {
				return result, fmt.Errorf("failed to get host by fqdn '%s': %w", instance.FQDN, err)
			}

			if d.GeoNames.matchGeo(host.Geo) {
				continue
			}

			dbaasResult, err := matchGeoDBaaSHost(mctx, mdb, host, d)
			if err != nil {
				return dbaasResult, err
			}
			result.matched = append(result.matched, dbaasResult.matched...)
			result.notfound = append(result.notfound, dbaasResult.notfound...)
		}
	}

	return result, nil
}

func matchGeoDBaaSHost(ctx context.Context, mdb metadb.MetaDB, host metadb.Host, d DrillConfig) (GeoMatchResult, error) {
	result := newGeoMatchResult()
	dscvr := metadbdiscovery.NewMetaDBBasedDiscovery(mdb)
	neighbours, err := dscvr.FindInShardOrSubcidByFQDN(ctx, host.FQDN)
	if err != nil {
		return result, err
	}

	for _, replica := range neighbours.Others {
		if d.GeoNames.matchGeo(replica.Geo) {
			result.matched = append(result.matched, fmt.Sprintf("%s role %s, subcid %s",
				replica.FQDN,
				strings.Join(replica.Roles, ", "),
				replica.SubClusterID,
			))
		}
	}
	return result, nil
}

func matchGeoConductorHost(ctx context.Context, cncl conductor.Client, host string, d DrillConfig) (GeoMatchResult, error) {
	result := newGeoMatchResult()
	hostsInfo, err := cncl.HostsInfo(ctx, []string{host})
	if err != nil {
		return result, fmt.Errorf("failed to get host info from conductor '%s': %w", host, err)
	}
	if len(hostsInfo) == 0 {
		result.notfound = append(result.notfound, fmt.Sprintf("%q not found in conductor", host))
		return result, nil
	}

	hostInfo := hostsInfo[0] // if len(hostsInfo) is 0 err would not be nil
	if d.GeoNames.matchGeo(hostInfo.DC) {
		return result, nil
	}

	replicas, err := cncl.GroupToHosts(ctx, hostInfo.Group, conductor.GroupToHostsAttrs{})
	if err != nil {
		return result, fmt.Errorf("failed to get group hosts from conductor '%s': %w", hostInfo.Group, err)
	}

	if len(replicas) == 1 {
		result.matched = append(result.matched, fmt.Sprintf("%s has not replicas in conductor group %s",
			host,
			hostInfo.Group,
		))
		return result, nil
	}

	hostsInfo, err = cncl.HostsInfo(ctx, replicas)
	if err != nil {
		return result, fmt.Errorf("failed to get hosts info from conductor '%s': %w", replicas, err)
	}

	for _, replica := range hostsInfo {
		if replica.FQDN != host && d.GeoNames.matchGeo(replica.DC) {
			result.matched = append(result.matched, fmt.Sprintf("%s, conductor group %s",
				replica.FQDN,
				replica.Group,
			))
		}
	}

	return result, nil
}

func matchDates(now time.Time, d DrillConfig) (bool, string) {
	before := d.From.Sub(now)
	if before > 0 && before <= d.WaitBefore.Duration {
		return true, fmt.Sprintf(
			"There will be drills in locations %s in %s, check https://st.yandex-team.ru/%s",
			strings.Join(d.GeoNames, ", "),
			before.Round(time.Second),
			d.Ticket,
		)
	}

	after := now.Sub(d.Till.Time)
	if before <= 0 && after <= d.WaitAfter.Duration {
		stopTime := d.Till.Add(d.WaitAfter.Duration)
		return true, fmt.Sprintf(
			"Drills in locations %s, wait %s till %s, check https://st.yandex-team.ru/%s",
			strings.Join(d.GeoNames, ", "),
			stopTime.Sub(now).Round(time.Second),
			stopTime.Round(time.Second).Format(time.RFC3339),
			d.Ticket,
		)
	}

	return false, ""
}

func formatResultMessage(msg string, addLog []string) string {
	if len(addLog) > 0 {
		return fmt.Sprintf("%s\nAdditional log:\n* %s",
			msg,
			strings.Join(addLog, "\n* "),
		)
	} else {
		return msg
	}
}

func newGeoMatchResult() GeoMatchResult {
	return GeoMatchResult{
		matched:  make([]string, 0),
		notfound: make([]string, 0),
	}
}

func NewTodayCheckDrills(cfg DrillsConfig, mdb metadb.MetaDB, dom0d dom0discovery.Dom0Discovery, cncl conductor.Client) DecisionStep {
	return NewCheckDrills(cfg, time.Now(), mdb, dom0d, cncl)
}

func NewCheckDrills(cfg DrillsConfig, today time.Time, mdb metadb.MetaDB, dom0 dom0discovery.Dom0Discovery, cncl conductor.Client) DecisionStep {
	return &CheckDrills{now: today, cfg: cfg, mdb: mdb, dom0: dom0, cncl: cncl}
}
