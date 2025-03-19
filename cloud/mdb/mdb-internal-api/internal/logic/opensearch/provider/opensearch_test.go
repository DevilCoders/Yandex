package provider

import (
	"context"
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
)

type openSearchFixture struct {
	testhelpers.Fixture
	OpenSearch *OpenSearch
}

func newOpenSearchFixture(t *testing.T) (context.Context, *openSearchFixture) {
	ctx, f := testhelpers.NewFixture(t)
	service := New(logic.Config{}, nil, f.Operator, f.Backups, f.Events, nil, f.Tasks, f.Compute, f.CryptoProvider, generator.NewRandomIDGenerator(), nil)
	return ctx, &openSearchFixture{Fixture: *f, OpenSearch: service}
}

func (f *openSearchFixture) Finish() {
	f.Ctrl.Finish()
}
