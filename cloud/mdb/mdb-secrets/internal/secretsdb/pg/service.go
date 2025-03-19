package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

type serviceImpl struct {
	cluster *sqlutil.Cluster
	log     log.Logger

	// encryption
	saltPublicKey [32]byte
	privateKey    [32]byte
}

// New constructs Service
func New(cfg pgutil.Config, l log.Logger, saltPublicKey, privateKey [32]byte) (secretsdb.Service, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}
	return &serviceImpl{
		cluster:       cluster,
		log:           l,
		saltPublicKey: saltPublicKey,
		privateKey:    privateKey,
	}, nil
}

// IsReady returns errors while storage is not ready
func (s *serviceImpl) IsReady(ctx context.Context) error {
	node := s.cluster.Alive()
	if node == nil {
		return secretsdb.ErrNotAvailable.WithFrame()
	}

	return nil
}
