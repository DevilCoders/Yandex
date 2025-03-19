package ydb

import (
	"context"
	"errors"
	"sort"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/meta"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (s *MetaSession) GetBillingAccounts(
	ctx context.Context, _ entities.ProcessingScope, ids []string,
) (result []entities.BillingAccount, err error) {
	if len(ids) == 0 {
		return
	}

	partsCnt := (len(ids)-1)/s.paramsBatchSize() + 1
	parts := make([][]string, partsCnt)
	results := make([][]entities.BillingAccount, partsCnt)
	for i, id := range ids {
		parts[i%partsCnt] = append(parts[i%partsCnt], id)
	}

	wg, ctx := errgroup.WithContext(ctx)
	for i := range parts {
		i := i
		part := parts[i]
		wg.Go(func() error {
			r, err := s.getBillingAccounts(ctx, part)
			if err == nil {
				results[i] = r
			}
			return err
		})
	}
	if err = wg.Wait(); err != nil {
		return
	}

	for _, r := range results {
		result = append(result, r...)
	}
	return
}

func (s *MetaSession) GetCloudBindings(
	ctx context.Context, _ entities.ProcessingScope, ids []entities.CloudAtTime,
) ([]entities.CloudBinding, error) {
	if len(ids) == 0 {
		return nil, nil
	}

	cloudRanges := make(idRangeMap)
	for _, id := range ids {
		rng, ok := cloudRanges[id.CloudID]
		if rng.to.Before(id.At) {
			rng.to = id.At
			ok = false
		}
		if rng.from.IsZero() || rng.from.After(id.At) {
			rng.from = id.At
			ok = false
		}
		if !ok {
			cloudRanges[id.CloudID] = rng
		}
	}

	dbBindings, err := s.getBindings(ctx, "cloud", cloudRanges)
	if err != nil {
		return nil, err
	}

	var result []entities.CloudBinding
	for _, r := range dbBindings {
		if r.ServiceInstanceType != "cloud" {
			return nil, ErrResourceBindingType.Wrap(errors.New(r.ServiceInstanceType))
		}
		result = append(result, entities.CloudBinding{
			CloudID:        r.ServiceInstanceID,
			BillingAccount: r.BillingAccountID,
			BindingTimes: entities.BindingTimes{
				EffectiveFrom: time.Time(r.EffectiveTime),
				EffectiveTo:   time.Time(r.EffectiveTo),
			},
		})
	}
	return result, nil
}

func (s *MetaSession) GetResourceBindings(
	ctx context.Context, _ entities.ProcessingScope, ids []entities.ResourceAtTime,
) ([]entities.ResourceBinding, error) {
	if len(ids) == 0 {
		return nil, nil
	}

	instanceRanges := make(map[string]idRangeMap)
	for _, id := range ids {
		rbt := encodeResourceBindingType(id.BindingType)
		if rbt == "" {
			return nil, ErrResourceBindingType.Wrap(errors.New(id.BindingType.String()))
		}

		typeMap := instanceRanges[rbt]
		if typeMap == nil {
			typeMap = make(idRangeMap)
		}
		rng, ok := typeMap[id.ResourceID]
		if rng.to.Before(id.At) {
			rng.to = id.At
			ok = false
		}
		if rng.from.IsZero() || rng.from.After(id.At) {
			rng.from = id.At
			ok = false
		}
		if !ok {
			typeMap[id.ResourceID] = rng
		}
		instanceRanges[rbt] = typeMap
	}

	types := make([]string, 0, len(instanceRanges))
	for rbt := range instanceRanges {
		types = append(types, rbt)
	}
	sort.Strings(types)

	wg, ctx := errgroup.WithContext(ctx)
	dbResults := make([][]meta.InstanceBindingRow, len(types))
	for i := range types {
		i := i
		rbt := types[i]
		wg.Go(func() error {
			dbBindings, err := s.getBindings(ctx, rbt, instanceRanges[rbt])
			if err == nil {
				dbResults[i] = dbBindings
			}
			return err
		})
	}
	if err := wg.Wait(); err != nil {
		return nil, err
	}

	var result []entities.ResourceBinding
	for _, dbBindings := range dbResults {
		for _, r := range dbBindings {
			bindType := decodeResourceBindingType(r.ServiceInstanceType)
			if bindType == entities.NoResourceBinding {
				return nil, ErrResourceBindingType.Wrap(errors.New(r.ServiceInstanceType))
			}

			result = append(result, entities.ResourceBinding{
				ResourceBindingKey: entities.ResourceBindingKey{
					ResourceID:  r.ServiceInstanceID,
					BindingType: bindType,
				},
				BillingAccount: r.BillingAccountID,
				BindingTimes: entities.BindingTimes{
					EffectiveFrom: time.Time(r.EffectiveTime),
					EffectiveTo:   time.Time(r.EffectiveTo),
				},
			})
		}
	}
	return result, nil
}

func (s *MetaSession) getBillingAccounts(
	ctx context.Context, ids []string,
) (result []entities.BillingAccount, err error) {
	var dbBAMapping []meta.BillingAccountMappingRow
	{
		retryCtx := tooling.StartRetry(ctx)
		err = s.retryRead(retryCtx, func() (resErr error) {
			tooling.RetryIteration(retryCtx)

			tx, err := s.adapter.db.BeginTxx(retryCtx, readCommitted())
			if err != nil {
				return err
			}
			defer func() {
				autoTx(tx, resErr)
			}()

			queries := meta.New(tx, s.adapter.queryParams)
			dbBAMapping, err = queries.GetBAMappingByID(retryCtx, ids...)
			return err
		})
	}
	if err != nil {
		return
	}

	result = make([]entities.BillingAccount, 0, len(dbBAMapping))
	for _, ba := range dbBAMapping {
		result = append(result, entities.BillingAccount{
			AccountID:       ba.ID,
			MasterAccountID: string(ba.MasterAccountID),
		})
	}
	return
}

func (s *MetaSession) getBindings(
	ctx context.Context, instanceType string, idRanges idRangeMap,
) (result []meta.InstanceBindingRow, err error) {
	sortedIDs := make([]string, 0, len(idRanges))
	for id := range idRanges {
		sortedIDs = append(sortedIDs, id)
	}
	sort.Strings(sortedIDs)

	partsCnt := (len(sortedIDs)-1)/s.paramsBatchSize() + 1
	parts := make([]idsAtTime, partsCnt)
	for i, id := range sortedIDs {
		rng := &parts[i%partsCnt]
		idRng := idRanges[id]
		if rng.to.Before(idRng.to) {
			rng.to = idRng.to
		}
		if rng.from.IsZero() || rng.from.After(idRng.from) {
			rng.from = idRng.from
		}
		rng.ids = append(rng.ids, id)
	}

	wg, ctx := errgroup.WithContext(ctx)
	results := make([][]meta.InstanceBindingRow, partsCnt)
	for i := range parts {
		i := i
		part := parts[i]
		wg.Go(func() error {
			retryCtx := tooling.StartRetry(ctx)
			return s.retryRead(retryCtx, func() (resErr error) {
				tooling.RetryIteration(retryCtx)

				tx, err := s.adapter.db.BeginTxx(retryCtx, readCommitted())
				if err != nil {
					return err
				}
				defer func() {
					autoTx(tx, resErr)
				}()

				queries := meta.New(tx, s.adapter.queryParams)
				dbResult, err := queries.GetBABindings(retryCtx, instanceType, part.from, part.to, part.ids...)
				if err == nil {
					results[i] = dbResult
				}
				return err
			})
		})
	}
	if err := wg.Wait(); err != nil {
		return nil, err
	}

	for _, r := range results {
		result = append(result, r...)
	}

	return
}

func encodeResourceBindingType(bt entities.ResourceBindingType) string {
	switch bt {
	case entities.TrackerResourceBinding:
		return "tracker"
	}
	return ""
}

func decodeResourceBindingType(v string) entities.ResourceBindingType {
	switch v {
	case "tracker":
		return entities.TrackerResourceBinding
	}
	return entities.NoResourceBinding
}

type timeRange struct {
	from, to time.Time
}

type idRangeMap map[string]timeRange

type idsAtTime struct {
	timeRange
	ids []string
}
