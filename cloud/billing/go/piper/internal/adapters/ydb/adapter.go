package ydb

import (
	"context"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"
)

type MetaAdapter struct {
	commonAdapter
	// various caches
	schemas schemasStore
	units   unitsStore

	muCumulative          sync.Mutex
	ensuredCumulativeLogs map[time.Time]bool
}

func NewMetaAdapter(runCtx context.Context, db *sqlx.DB, schemeDB SchemeSessionGetter, database, rootPath string) (*MetaAdapter, error) {
	adapter := &MetaAdapter{
		commonAdapter: newCommonAdapter(runCtx, db, schemeDB, database, rootPath),
	}
	return adapter, adapter.init()
}

func (a *MetaAdapter) init() error {
	a.ensuredCumulativeLogs = make(map[time.Time]bool)
	return nil
}

type MetaSession struct {
	commonSession

	adapter *MetaAdapter

	batchSizeOverride int

	oversizedBuffer oversizedStore
	invalidBuffer   invalidStore

	schemas schemasData
	units   unitsData
}

func (a *MetaAdapter) Session() *MetaSession {
	return &MetaSession{adapter: a}
}

func (s *MetaSession) paramsBatchSize() int {
	if s.batchSizeOverride > 0 {
		return s.batchSizeOverride
	}
	return paramsBatchSize
}

type DataAdapter struct {
	// NOTE: For now assume all methods with custom data work should be placed here

	commonAdapter
	DataAdapterConfig

	uniq uniqCache
}

func NewDataAdapter(
	runCtx context.Context, config DataAdapterConfig, db *sqlx.DB, schemeDB SchemeSessionGetter, database, rootPath string,
) (*DataAdapter, error) {
	adapter := &DataAdapter{
		commonAdapter:     newCommonAdapter(runCtx, db, schemeDB, database, rootPath),
		DataAdapterConfig: config,
	}
	return adapter, nil
}

type DataAdapterConfig struct {
	EnableUniq      bool
	EnablePresenter bool
}
