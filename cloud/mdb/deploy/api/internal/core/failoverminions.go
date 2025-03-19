package core

import (
	"context"
	"time"
)

func (srv *Service) backgroundFailoverMinions(ctx context.Context) {
	srv.lg.Debug("Started minions failover routine")
	defer srv.lg.Debug("Stopped minions failover routine")

	ticker := time.NewTicker(srv.cfg.FailoverMinionsPeriod)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			minions, err := srv.ddb.FailoverMinions(ctx, srv.cfg.FailoverMinionsSize)
			if err != nil {
				srv.lg.Errorf("Failed to failover minions: %s", err)
				continue
			}

			for _, m := range minions {
				srv.lg.Infof("Failover done for minion %q to master %q", m.FQDN, m.MasterFQDN)
			}
		}
	}
}
