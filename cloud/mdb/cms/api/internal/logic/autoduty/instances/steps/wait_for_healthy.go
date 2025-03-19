package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/dustin/go-humanize"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	healthapi "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrClusterStopped = xerrors.NewSentinel("cluster is STOPPED")
)

type WaitForHealthy struct {
	health healthapi.MDBHealthClient
	rr     healthdbspec.RoleSpecificResolvers
	meta   metadb.MetaDB
}

func (s WaitForHealthy) Name() string {
	return "health of cluster"
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
		"%sHA: %v%s, roles: %s, giving away this node will leave %d healthy nodes of %d total, space limit %s",
		hint,
		ha,
		haSpecific,
		rolesPlayed,
		fc.CntAliveLeftInGroup,
		fc.CntTotalInGroup,
		humanize.Bytes(uint64(fc.Instance.VolumesTotalSpaceLimit())),
	)
}

func (s WaitForHealthy) RunStep(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) RunResult {
	mdbHealth := healthbased.NewHealthBasedHealthiness(s.health, s.rr, time.Now())
	if stepCtx.State().WaitForHealthyStep.Checked {
		return continueWithMessage("health has been checked already")
	}

	fqdn := stepCtx.FQDN()
	if err := s.checkClusterStatus(ctx, fqdn); err != nil {
		if xerrors.Is(err, ErrClusterStopped) {
			stepCtx.State().WaitForHealthyStep.Checked = true
			return continueWithMessage("cluster is STOPPED, do not need to check health")
		}
		return waitWithErrAndMessage(err, "can not check cluster status")
	}

	ctxlog.Debug(ctx, l, "request health info about host neighbours", log.String("FQDN", fqdn))
	health, err := mdbHealth.ByInstances(ctx, []models.Instance{
		{FQDN: fqdn},
	})
	if err != nil {
		return waitWithErrAndMessage(err, "can not request neighbours info")
	}

	ctxlog.Debug(ctx, l, "got response", log.Any("neighboursInfo", health))

	if len(health.Unknown) > 0 {
		stepCtx.State().WaitForHealthyStep.Checked = true
		return continueWithMessageFmt("health knows nothing about %s", fqdn)
	}

	if len(health.WouldDegrade) > 0 {
		return waitWithMessageFmt("would degrade now:\n%s", HealthToString(health.WouldDegrade[0]))
	}

	if len(health.Stale) > 0 {
		return waitWithMessageFmt("outdated info about:\n%s", HealthToString(health.Stale[0]))
	}

	stepCtx.State().WaitForHealthyStep.Checked = true
	return continueWithMessage("all neighbours are healthy")
}

func (s WaitForHealthy) checkClusterStatus(ctx context.Context, fqdn string) error {
	txCtx, err := s.meta.Begin(ctx, sqlutil.Alive)
	if err != nil {
		return xerrors.Errorf("metadb begin tx: %w", err)
	}
	defer func() { _ = s.meta.Rollback(txCtx) }()

	host, err := s.meta.GetHostByFQDN(txCtx, fqdn)
	if err != nil {
		return xerrors.Errorf("host by fqdn: %w", err)
	}

	cluster, err := s.meta.ClusterInfo(txCtx, host.ClusterID)
	if err != nil {
		return xerrors.Errorf("cluster by id: %w", err)
	}

	if cluster.Status == metadb.ClusterStatusStopped {
		return ErrClusterStopped
	}

	return nil
}

func NewWaitForHealthy(health healthapi.MDBHealthClient, meta metadb.MetaDB) WaitForHealthy {
	return WaitForHealthy{
		health: health,
		rr:     healthdbspec.NewRoleSpecificResolver(),
		meta:   meta,
	}
}
