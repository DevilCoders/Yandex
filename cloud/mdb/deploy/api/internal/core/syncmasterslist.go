package core

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func (srv *Service) backgroundMastersListSync(ctx context.Context) {
	ticker := time.NewTicker(srv.cfg.SyncMastersListPeriod)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			srv.syncMastersList(ctx)
		}
	}
}

func (srv *Service) syncMastersList(ctx context.Context) {
	oldMasters := srv.masters.Load().(map[string]*knownMaster)
	newMasters := map[string]*knownMaster{}
	var lastID models.MasterID

	span, ctxWithSpan := opentracing.StartSpanFromContext(ctx, "SyncMastersList")
	defer span.Finish()

	for {
		loaded, err := srv.ddb.Masters(ctxWithSpan, srv.cfg.SyncMastersListPageSize, optional.NewInt64(int64(lastID)))
		if err != nil {
			srv.lg.Errorf("Failed to retrieve masters: %s", err)
			return
		}

		// Pagination ended
		if len(loaded) == 0 {
			break
		}

		for _, m := range loaded {
			// Pagination
			if m.ID > lastID {
				lastID = m.ID
			}

			if old, ok := oldMasters[m.FQDN]; ok {
				old.StoreInfo(m)
				newMasters[m.FQDN] = old
				continue
			}

			masterAddr := m.FQDN
			if srv.cfg.RestrictToLocalIdentifier != "" {
				var masterLocalityIdentifier string
				masterAddr, masterLocalityIdentifier, err = localityIdentifierFromFQDN(m.FQDN)
				if err != nil {
					srv.lg.Errorf("locality identifier for master %q: %s", m.FQDN, err)
					continue
				}

				if masterLocalityIdentifier != srv.cfg.RestrictToLocalIdentifier {
					srv.lg.Debugf("locality identifier for master %q differ from ours: %q vs %q",
						m.FQDN, masterLocalityIdentifier, srv.cfg.RestrictToLocalIdentifier)
					continue
				}
			}

			srv.lg.Debugf("Detected new salt master %q with addr %q", m.FQDN, masterAddr)

			// ATTENTION: It is crucial that we use 'spanless' context here because otherwise all spans for
			// created master will be children of span of this check when in reality they must be 'top-level' spans.
			km, err := newKnownMaster(ctx, m, masterAddr, srv.cfg, srv.ddb, srv.lg)
			if err != nil {
				srv.lg.Errorf("Failed to create known master: %s", err)
				continue
			}

			newMasters[m.FQDN] = km
		}
	}

	// Cancel autoauth on now unknown masters
	for fqdn, m := range oldMasters {
		if _, ok := newMasters[fqdn]; !ok {
			srv.lg.Debugf("Salt master %q is not known anymore. Stopping its background jobs.", fqdn)
			m.cancel()
		}
	}

	srv.masters.Store(newMasters)
}
