package actions

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

type BillingAccountsGetter interface {
	GetBillingAccounts(
		ctx context.Context, scope entities.ProcessingScope, ids []string,
	) ([]entities.BillingAccount, error)

	GetCloudBindings(
		ctx context.Context, scope entities.ProcessingScope, ids []entities.CloudAtTime,
	) ([]entities.CloudBinding, error)

	GetResourceBindings(
		ctx context.Context, scope entities.ProcessingScope, ids []entities.ResourceAtTime,
	) ([]entities.ResourceBinding, error)
}

type FoldersGetter interface {
	GetFolders(
		ctx context.Context, scope entities.ProcessingScope, ids []string,
	) ([]entities.Folder, error)
}

type AbcResolver interface {
	ResolveAbc(
		ctx context.Context, scope entities.ProcessingScope, ids []int64,
	) ([]entities.AbcFolder, error)
}

func ResolveMetricsIdentity(
	ctx context.Context, scope entities.ProcessingScope, ba BillingAccountsGetter, folders FoldersGetter,
	abc AbcResolver,
	metrics []entities.EnrichedMetric,
) (valid []entities.EnrichedMetric, unresolved []entities.InvalidMetric, err error) {
	ctx = tooling.ActionStarted(ctx)
	defer func() {
		tooling.ActionDone(ctx, err)
		if err == nil {
			tooling.InvalidMetrics(ctx, unresolved)
		}
	}()

	p := identityProcessor{
		enrichedCommon: enrichedCommon{},
		ctx:            ctx,
		scope:          scope,
		ba:             ba,
		folders:        folders,
		abc:            abc,
	}

	return p.resolve(metrics)
}

type identityProcessor struct {
	enrichedCommon

	ctx   context.Context
	scope entities.ProcessingScope

	ba      BillingAccountsGetter
	folders FoldersGetter
	abc     AbcResolver
}

func (ip *identityProcessor) resolve(
	metrics []entities.EnrichedMetric,
) (valid []entities.EnrichedMetric, unresolved []entities.InvalidMetric, err error) {
	var (
		toBAResolve       []*entities.EnrichedMetric
		toFolderResolve   []*entities.EnrichedMetric
		toCloudResolve    []*entities.EnrichedMetric
		toABCResolve      []*entities.EnrichedMetric
		toResourceResolve []*entities.EnrichedMetric
	)
	bannedMap := make(map[*entities.EnrichedMetric]struct{})

	// Distribute mertic pointer to resolvers
	for i := range metrics {
		m := &metrics[i]

		switch {
		case m.BillingAccountID != "":
			toBAResolve = append(toBAResolve, m)
		case m.CloudID != "":
			toCloudResolve = append(toCloudResolve, m)
			toBAResolve = append(toBAResolve, m)
		case m.FolderID != "":
			toFolderResolve = append(toFolderResolve, m)
			toCloudResolve = append(toCloudResolve, m)
			toBAResolve = append(toBAResolve, m)
		case m.AbcFolderID != "":
			toABCResolve = append(toABCResolve, m)
			toCloudResolve = append(toCloudResolve, m)
			toBAResolve = append(toBAResolve, m)
		case m.ResourceID != "" && m.ResourceBindingType != entities.NoResourceBinding:
			toResourceResolve = append(toResourceResolve, m)
			toBAResolve = append(toBAResolve, m)
		default:
			bannedMap[m] = struct{}{}
		}
	}

	{
		ip.banMetrics(&toABCResolve, bannedMap)
		notFound, resolveErr := ip.resolveABC(toABCResolve)
		if resolveErr != nil {
			return nil, nil, resolveErr
		}
		ip.addBanned(bannedMap, notFound)
	}

	{
		ip.banMetrics(&toFolderResolve, bannedMap)
		notFound, resolveErr := ip.resolveFolders(toFolderResolve)
		if resolveErr != nil {
			return nil, nil, resolveErr
		}
		ip.addBanned(bannedMap, notFound)
	}

	{
		ip.banMetrics(&toCloudResolve, bannedMap)
		notFound, resolveErr := ip.resolveClouds(toCloudResolve)
		if resolveErr != nil {
			return nil, nil, resolveErr
		}
		ip.addBanned(bannedMap, notFound)
	}

	{
		ip.banMetrics(&toResourceResolve, bannedMap)
		notFound, resolveErr := ip.resolveResources(toResourceResolve)
		if resolveErr != nil {
			return nil, nil, resolveErr
		}
		ip.addBanned(bannedMap, notFound)
	}

	{
		ip.banMetrics(&toBAResolve, bannedMap)
		notFound, resolveErr := ip.resolveBillingAccounts(toBAResolve)
		if resolveErr != nil {
			return nil, nil, resolveErr
		}
		ip.addBanned(bannedMap, notFound)
	}

	for i := range metrics {
		m := &metrics[i]

		if _, banned := bannedMap[m]; !banned {
			valid = append(valid, *m)
			continue
		}

		im := ip.makeIncorrectMetric(
			*m, entities.FailedByBillingAccountResolving, fmt.Sprintf(
				"Cannot resolve billing account for billing_account_id=%s, "+
					"cloud_id=%s, folder_id=%s, abc_folder_id=%s, resource_id=%s",
				m.BillingAccountID, m.CloudID, m.FolderID, m.AbcFolderID, m.ResourceID,
			),
		)
		unresolved = append(unresolved, im)
	}
	return
}

func (ip *identityProcessor) resolveABC(metrics []*entities.EnrichedMetric) ([]*entities.EnrichedMetric, error) {
	unq := make(map[int64]struct{})
	for _, m := range metrics {
		unq[m.AbcID] = struct{}{}
	}
	ids := ip.intSliceFromMap(unq)
	folders, err := ip.abc.ResolveAbc(ip.ctx, ip.scope, ids)
	if err != nil {
		return nil, err
	}

	foldersMap := make(map[int64]entities.AbcFolder, len(folders))
	for _, f := range folders {
		foldersMap[f.AbcID] = f
	}

	var unresolved []*entities.EnrichedMetric
	for _, m := range metrics {
		rec := foldersMap[m.AbcID]
		if rec.CloudID == "" {
			unresolved = append(unresolved, m)
		}
		m.CloudID = rec.CloudID
		m.AbcFolderID = rec.AbcFolderID
	}
	return unresolved, nil
}

func (ip *identityProcessor) resolveFolders(metrics []*entities.EnrichedMetric) ([]*entities.EnrichedMetric, error) {
	unq := make(map[string]struct{})
	for _, m := range metrics {
		unq[m.FolderID] = struct{}{}
	}
	ids := ip.sliceFromMap(unq)
	folders, err := ip.folders.GetFolders(ip.ctx, ip.scope, ids)
	if err != nil {
		return nil, err
	}

	clouds := make(map[string]string, len(folders))
	for _, f := range folders {
		clouds[f.FolderID] = f.CloudID
	}

	var unresolved []*entities.EnrichedMetric
	for _, m := range metrics {
		m.CloudID = clouds[m.FolderID]
		if m.CloudID == "" {
			unresolved = append(unresolved, m)
		}
	}
	return unresolved, nil
}

func (ip *identityProcessor) resolveClouds(metrics []*entities.EnrichedMetric) ([]*entities.EnrichedMetric, error) {
	unq := make(map[entities.CloudAtTime]struct{})
	ids := []entities.CloudAtTime{}
	for _, m := range metrics {
		key := entities.CloudAtTime{
			CloudID: m.CloudID,
			At:      m.Usage.UsageTime(),
		}
		_, found := unq[key]
		if found {
			continue
		}
		unq[key] = struct{}{}
		ids = append(ids, key)
	}

	bindings, err := ip.ba.GetCloudBindings(ip.ctx, ip.scope, ids)
	if err != nil {
		return nil, err
	}

	accs := make(map[string][]entities.CloudBinding)
	for _, b := range bindings {
		accs[b.CloudID] = append(accs[b.CloudID], b)
	}

	var unresolved []*entities.EnrichedMetric
	for _, m := range metrics {
		ut := m.Usage.UsageTime()
	RESOLVE_LOOP:
		for _, bnd := range accs[m.CloudID] {
			if bnd.Belongs(ut) {
				m.BillingAccountID = bnd.BillingAccount
				break RESOLVE_LOOP
			}
		}

		if m.BillingAccountID == "" {
			unresolved = append(unresolved, m)
		}
	}
	return unresolved, nil
}

func (ip *identityProcessor) resolveResources(metrics []*entities.EnrichedMetric) ([]*entities.EnrichedMetric, error) {
	unq := make(map[entities.ResourceAtTime]struct{})
	ids := []entities.ResourceAtTime{}
	for _, m := range metrics {
		key := entities.ResourceAtTime{
			ResourceBindingKey: entities.ResourceBindingKey{
				ResourceID:  m.ResourceID,
				BindingType: m.ResourceBindingType,
			},
			At: m.Usage.UsageTime(),
		}
		_, found := unq[key]
		if found {
			continue
		}
		unq[key] = struct{}{}
		ids = append(ids, key)
	}

	bindings, err := ip.ba.GetResourceBindings(ip.ctx, ip.scope, ids)
	if err != nil {
		return nil, err
	}

	accs := make(map[entities.ResourceBindingKey][]entities.ResourceBinding)
	for _, b := range bindings {
		accs[b.ResourceBindingKey] = append(accs[b.ResourceBindingKey], b)
	}

	var unresolved []*entities.EnrichedMetric
	for _, m := range metrics {
		ut := m.Usage.UsageTime()
		bk := entities.ResourceBindingKey{
			ResourceID:  m.ResourceID,
			BindingType: m.ResourceBindingType,
		}
	RESOLVE_LOOP:
		for _, bnd := range accs[bk] {
			if bnd.Belongs(ut) {
				m.BillingAccountID = bnd.BillingAccount
				break RESOLVE_LOOP
			}
		}

		if m.BillingAccountID == "" {
			unresolved = append(unresolved, m)
		}
	}
	return unresolved, nil
}

func (ip *identityProcessor) resolveBillingAccounts(metrics []*entities.EnrichedMetric) ([]*entities.EnrichedMetric, error) {
	unq := make(map[string]struct{})
	for _, m := range metrics {
		unq[m.BillingAccountID] = struct{}{}
	}
	ids := ip.sliceFromMap(unq)
	accs, err := ip.ba.GetBillingAccounts(ip.ctx, ip.scope, ids)
	if err != nil {
		return nil, err
	}

	ba := make(map[string]string, len(accs))
	for _, a := range accs {
		ba[a.AccountID] = a.MasterAccountID
	}

	var unresolved []*entities.EnrichedMetric
	for _, m := range metrics {
		rslv := false
		m.MasterAccountID, rslv = ba[m.BillingAccountID]
		if !rslv {
			unresolved = append(unresolved, m)
		}
		m.ReshardingKey = m.MasterAccountID
		if m.ReshardingKey == "" {
			m.ReshardingKey = m.BillingAccountID
		}
	}
	return unresolved, nil
}

func (identityProcessor) banMetrics(list *([]*entities.EnrichedMetric), bans map[*entities.EnrichedMetric]struct{}) {
	result := (*list)[:0]
	for _, m := range *list {
		_, banned := bans[m]
		if !banned {
			result = append(result, m)
		}
	}
	*list = result
}

func (identityProcessor) addBanned(bans map[*entities.EnrichedMetric]struct{}, notFound []*entities.EnrichedMetric) {
	for _, m := range notFound {
		bans[m] = struct{}{}
	}
}

func (identityProcessor) sliceFromMap(m map[string]struct{}) []string {
	r := make([]string, 0, len(m))
	for k := range m {
		r = append(r, k)
	}
	return r
}

func (identityProcessor) intSliceFromMap(m map[int64]struct{}) []int64 {
	r := make([]int64, 0, len(m))
	for k := range m {
		r = append(r, k)
	}
	return r
}
