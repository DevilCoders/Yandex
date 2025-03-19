package provider

import (
	"context"
	"testing"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
)

type clickHouseFixture struct {
	*testhelpers.Fixture
	ClickHouse *ClickHouse
}

func newClickHouseFixture(t *testing.T) (context.Context, clickHouseFixture) {
	ctx, f := testhelpers.NewFixture(t)
	ch := New(logic.Config{}, f.Operator, f.Backups, f.Events, f.Search, f.Tasks, f.Compute, f.CryptoProvider, nil, nil)
	return ctx, clickHouseFixture{Fixture: f, ClickHouse: ch}
}
