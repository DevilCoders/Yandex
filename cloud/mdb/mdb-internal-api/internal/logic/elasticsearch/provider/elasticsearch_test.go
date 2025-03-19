package provider

import (
	"context"
	"testing"

	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/testhelpers"
)

type elasticsearchFixture struct {
	testhelpers.Fixture
	Elasticsearch *ElasticSearch
}

func newElasticsearchFixture(t *testing.T) (context.Context, *elasticsearchFixture) {
	ctx, f := testhelpers.NewFixture(t)
	service := New(logic.Config{}, nil, f.Operator, f.Backups, f.Events, nil, f.Tasks, f.Compute, f.CryptoProvider, generator.NewRandomIDGenerator(), nil)
	return ctx, &elasticsearchFixture{Fixture: *f, Elasticsearch: service}
}

func (f *elasticsearchFixture) Finish() {
	f.Ctrl.Finish()
}
