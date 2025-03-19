package ydb

import (
	"context"
	"encoding/json"
	"sort"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/meta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
)

type schemasStore struct {
	baseStore
	schemasData
}

type schemasData struct {
	tags       []schemaTags
	namesIndex map[string]int
}

type schemaTags struct {
	Required []string
}

func (ss *schemasStore) getData(ctx context.Context, db *sqlx.DB, qp qtool.QueryParams) (schemasData, error) {
	ss.mu.Lock()
	defer ss.mu.Unlock()

	if ss.consistent() {
		return ss.schemasData, nil
	}
	if ss.expired() {
		ss.schemasData = schemasData{}
	}

	err := ss.getSchemas(ctx, db, qp)
	if err == nil {
		ss.updated()
	}

	return ss.schemasData, err
}

func (ss *schemasStore) getSchemas(ctx context.Context, db *sqlx.DB, qp qtool.QueryParams) (resErr error) {
	tx, err := db.BeginTxx(ctx, readCommitted())
	if err != nil {
		return ss.selectSentinel(ErrSchemasExpired, ErrSchemasRefresh).Wrap(err)
	}
	defer func() {
		autoTx(tx, resErr)
	}()

	queries := meta.New(tx, qp)
	rows, err := queries.GetAllSchemas(ctx)
	if err != nil {
		return ss.selectSentinel(ErrSchemasExpired, ErrSchemasRefresh).Wrap(err)
	}

	ss.tags = make([]schemaTags, 0, len(rows))
	ss.namesIndex = make(map[string]int)
	for idx, sch := range rows {
		var tags schemaTags
		if err := json.Unmarshal([]byte(sch.Tags), &tags); err != nil {
			return ErrSchemaData.Wrap(err)
		}
		ss.tags = append(ss.tags, tags)
		ss.namesIndex[sch.Name] = idx
	}
	return nil
}

type unitsStore struct {
	baseStore
	unitsData
}

type keyPair struct {
	src, dst string
}

type unitsData struct {
	units       []meta.UnitRow
	conversions map[keyPair][]meta.ConversionRateRow
	unitsIndex  map[keyPair]int
}

func (us *unitsStore) getData(ctx context.Context, db *sqlx.DB, qp qtool.QueryParams, force bool) (unitsData, error) {
	us.mu.Lock()
	defer us.mu.Unlock()

	if us.consistent() && !force {
		return us.unitsData, nil
	}
	if us.expired() {
		us.unitsData = unitsData{}
	}

	err := us.getUnits(ctx, db, qp)
	if err == nil {
		us.updated()
		return us.unitsData, nil
	}

	return us.unitsData, err
}

func (us *unitsStore) getUnits(ctx context.Context, db *sqlx.DB, qp qtool.QueryParams) (resErr error) {
	tx, err := db.BeginTxx(ctx, readCommitted())
	if err != nil {
		return us.selectSentinel(ErrUnitsExpired, ErrUnitsRefresh).Wrap(err)
	}
	defer func() {
		autoTx(tx, resErr)
	}()

	queries := meta.New(tx, qp)
	unitRows, err := queries.GetAllUnits(ctx)
	if err != nil {
		return us.selectSentinel(ErrUnitsExpired, ErrUnitsRefresh).Wrap(err)
	}
	ratesRows, err := queries.GetAllConversionRates(ctx)
	if err != nil {
		return us.selectSentinel(ErrUnitsExpired, ErrUnitsRefresh).Wrap(err)
	}

	us.unitsIndex = make(map[keyPair]int)
	us.units = unitRows
	us.conversions = make(map[keyPair][]meta.ConversionRateRow)
	for idx, u := range unitRows {
		us.unitsIndex[keyPair{u.SrcUnit, u.DstUnit}] = idx
	}

	sort.Slice(ratesRows, func(i, j int) bool {
		return time.Time(ratesRows[i].EffectiveTime).Before(time.Time(ratesRows[j].EffectiveTime))
	})
	for _, cr := range ratesRows {
		key := keyPair{cr.SourceCurrency, cr.TargetCurrency}
		us.conversions[key] = append(us.conversions[key], cr)
	}
	return nil
}
