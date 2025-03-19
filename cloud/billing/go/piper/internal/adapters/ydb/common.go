package ydb

import (
	"context"
	"database/sql"
	"strings"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	nanDecimal      = decimal.Must(decimal.FromString("NaN"))
	paramsBatchSize = 1000
)

var (
	secInBaseMonth = func() decimal.Decimal128 {
		sec := int64((time.Hour * 24 * 30).Seconds())
		return decimal.Must(decimal.FromInt64(sec))
	}()
)

const (
	storesRefreshRate      = time.Minute * 5
	storesCriticalDuration = time.Minute * 10
)

type commonAdapter struct {
	runCtx context.Context

	db          *sqlx.DB
	schemeDB    SchemeSessionGetter
	queryParams qtool.QueryParams
}

func newCommonAdapter(runCtx context.Context, db *sqlx.DB, schemeDB SchemeSessionGetter, database, rootPath string) commonAdapter {
	if rootPath != "" && !strings.HasSuffix(rootPath, "/") {
		rootPath = rootPath + "/"
	}

	return commonAdapter{
		runCtx:   runCtx,
		db:       db,
		schemeDB: schemeDB,
		queryParams: qtool.QueryParams{
			RootPath: rootPath,
			DB:       database,
		},
	}
}

type commonSession struct {
	retrier
	nowOverride func() time.Time
}

func (s *commonSession) now() time.Time {
	if s.nowOverride != nil {
		return s.nowOverride()
	}
	return time.Now()
}

type baseStore struct {
	mu sync.Mutex

	updatedAt time.Time
}

func (s *baseStore) consistent() bool {
	return time.Since(s.updatedAt) < storesRefreshRate
}

func (s *baseStore) updated() {
	s.updatedAt = time.Now()
}

func (s *baseStore) expired() bool {
	return time.Since(s.updatedAt) > storesCriticalDuration
}

func (s *baseStore) selectSentinel(expired, notExpired *errsentinel.Sentinel) *errsentinel.Sentinel {
	if s.expired() {
		return expired
	}
	return notExpired
}

func readCommitted() *sql.TxOptions {
	return &sql.TxOptions{ReadOnly: true, Isolation: sql.LevelReadCommitted}
}

func serializable() *sql.TxOptions {
	return &sql.TxOptions{ReadOnly: false, Isolation: sql.LevelSerializable}
}

func autoTx(tx *sqlx.Tx, err error) {
	if err != nil {
		_ = tx.Rollback()
	}
	_ = tx.Commit()
}
