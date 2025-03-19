package provider

import (
	"context"
	"testing"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

type kafkaFixture struct {
	*testhelpers.Fixture
	Kafka   *Kafka
	Context context.Context
	Session sessions.Session
	T       *testing.T
}

func newKafkaFixture(t *testing.T) kafkaFixture {
	ctx, f := testhelpers.NewFixture(t)
	domainConfig := api.DomainConfig{
		Public:  "yadc.io",
		Private: "private.yadc.io",
	}
	config := logic.Config{
		SaltEnvs: logic.SaltEnvsConfig{
			Prestable:  environment.SaltEnvDev,
			Production: environment.SaltEnvComputeProd,
		},
	}
	kafka := New(config, domainConfig, f.Operator, f.Backups, f.Events, f.Search, f.Tasks, f.CryptoProvider, f.Compute, f.PillarSecrets, nil)
	return kafkaFixture{
		Fixture: f,
		Kafka:   kafka,
		Context: ctx,
		T:       t,
		Session: sessions.Session{},
	}
}
