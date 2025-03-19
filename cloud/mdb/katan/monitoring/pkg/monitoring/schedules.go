package monitoring

import (
	"context"
	"fmt"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (m *Monrun) scheduleFailReason(ctx context.Context, scheduleID int64, uiURI string) string {
	clusterRolls, err := m.kdb.ClusterRolloutsFailedInSchedule(ctx, scheduleID)
	if err != nil {
		m.L.Warnf("failed to get cluster rollouts: %s", err)
		return fmt.Sprintf("Additional information is not avaliable due: %s", err)
	}
	var brokenClusters []string
	for _, clusterRoll := range clusterRolls {
		desc := []string{
			fmt.Sprintf("'%s' cluster", clusterRoll.ClusterID),
		}

		if clusterRoll.Comment.Valid {
			desc = append(desc, fmt.Sprintf("cause: %s.", clusterRoll.Comment.String))
		}

		clusterShipments, err := m.kdb.RolloutShipmentsByCluster(ctx, clusterRoll.RolloutID, clusterRoll.ClusterID)
		if err != nil {
			m.L.Warnf("failed to get rollout %d shipments by cluster %q: %s", clusterRoll.RolloutID, clusterRoll.ClusterID, err)
		} else {
			// choose last shipment, cause
			var lastShip katandb.RolloutShipment
			for _, s := range clusterShipments {
				if s.ShipmentID > lastShip.ShipmentID {
					lastShip = s
				}
			}
			var prettyShipmentID string
			if len(uiURI) > 0 {
				prettyShipmentID = fmt.Sprintf("%s/deploy/shipment/%d", uiURI, lastShip.ShipmentID)
			} else {
				prettyShipmentID = strconv.FormatInt(lastShip.ShipmentID, 10)
			}
			desc = append(desc, fmt.Sprintf("Shipment: %s ", prettyShipmentID))
		}

		brokenClusters = append(brokenClusters, strings.Join(desc, " "))

	}

	if len(brokenClusters) == 0 {
		return "Schedule fails reason undefined"
	}
	return fmt.Sprintf("It fails on %d clusters. [%s]", len(brokenClusters), strings.Join(brokenClusters, ", "))
}

func (m *Monrun) CheckBrokenSchedules(ctx context.Context, namespace, uiURI string) monrun.Result {
	schedules, err := m.kdb.Schedules(ctx)

	if err != nil {
		if xerrors.Is(err, katandb.ErrNoDataFound) {
			return monrun.Result{}
		}
		if pgerrors.IsTemporary(err) {
			return monrun.Warnf("temporary DB error: %s", err)
		}
		return monrun.Critf("unexpected DB error: %s", err)
	}

	var broken []string
	for _, s := range schedules {
		if s.Namespace != namespace {
			continue
		}

		if s.State == katandb.ScheduleStateBroken {
			broken = append(
				broken,
				fmt.Sprintf("%s schedule (id: %d) is broken. %s", s.Name, s.ID, m.scheduleFailReason(ctx, s.ID, uiURI)),
			)
		}
	}
	switch len(broken) {
	case 0:
		return monrun.Result{}
	case 1:
		return monrun.Result{
			Code:    monrun.CRIT,
			Message: broken[0],
		}
	}
	return monrun.Critf("%d schedules are broken. %s", len(broken), strings.Join(broken, " "))
}
