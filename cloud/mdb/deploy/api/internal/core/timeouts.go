package core

import (
	"context"
	"time"
)

func (srv *Service) backgroundJobsTimeout(ctx context.Context) {
	timer := time.NewTimer(srv.cfg.CommandTimeoutPeriod)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			jobs, err := srv.ddb.TimeoutJobs(ctx, srv.cfg.CommandTimeoutPageSize)
			if err != nil {
				srv.lg.Errorf("Failed to timeout jobs: %s", err)
				timer.Reset(srv.cfg.CommandTimeoutPeriod)
				continue
			}

			// TODO: https://st.yandex-team.ru/MDB-4439
			for _, job := range jobs {
				srv.lg.Infof("Timed out job %+v", job)
			}

			if len(jobs) > 0 {
				timer.Reset(0)
				continue
			}

			timer.Reset(srv.cfg.CommandTimeoutPeriod)
		}
	}
}

func (srv *Service) backgroundShipmentsTimeout(ctx context.Context) {
	timer := time.NewTimer(srv.cfg.ShipmentTimeoutPeriod)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}
			return
		case <-timer.C:
			shipments, err := srv.ddb.TimeoutShipments(ctx, srv.cfg.ShipmentTimeoutPageSize)
			if err != nil {
				srv.lg.Errorf("Failed to timeout shipments: %s", err)
				timer.Reset(srv.cfg.ShipmentTimeoutPeriod)
				continue
			}

			for _, shipment := range shipments {
				srv.lg.Infof("Timed out shipment %+v", shipment)
			}

			if len(shipments) > 0 {
				timer.Reset(0)
				continue
			}

			timer.Reset(srv.cfg.ShipmentTimeoutPeriod)
		}
	}
}
