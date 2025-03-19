package teamintegration

import (
	"context"
	"sync"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	team_integration_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/ctxtool"
)

func (s *Session) ResolveAbc(
	ctx context.Context, _ entities.ProcessingScope, ids []int64,
) ([]entities.AbcFolder, error) {
	return s.adapter.abc.resolve(ctxtool.WithGlobalCancel(ctx, s.adapter.runCtx), s.adapter.tiClient, ids)
}

type abcResolveStorage struct {
	mu sync.RWMutex

	resolves map[int64]team_integration_ic.ResolvedFolder
}

func (s *abcResolveStorage) resolve(
	ctx context.Context, tiClient team_integration_ic.TIClient, ids []int64,
) ([]entities.AbcFolder, error) {
	var onReturn func()
	defer func() {
		if onReturn != nil {
			onReturn()
		}
	}()

	s.mu.RLock()
	onReturn = s.mu.RUnlock

	// check if storage contains all folders
	var hasUnknown bool
	for _, id := range ids {
		if _, ok := s.resolves[id]; !ok {
			hasUnknown = true
			break
		}
	}

	// if there are folders that missed in storage we should fetch them.
	if hasUnknown {
		s.mu.RUnlock()
		onReturn = nil
		s.mu.Lock()
		onReturn = s.mu.Unlock

		// NOTE: some folders could be fetched while we wait for lock
		// so we pass all ids to fetch and filter them inside.
		if err := s.fetch(ctx, tiClient, ids); err != nil {
			return nil, err
		}
	}

	return s.resolveSync(ids), nil
}

func (s *abcResolveStorage) resolveSync(ids []int64) (result []entities.AbcFolder) {
	result = make([]entities.AbcFolder, 0, len(ids))
	for _, id := range ids {
		rsl := s.resolves[id]
		result = append(result, entities.AbcFolder{
			AbcID:       id,
			AbcFolderID: rsl.FolderID,
			CloudID:     rsl.CloudID,
		})
	}
	return
}

func (s *abcResolveStorage) fetch(
	ctx context.Context, tiClient team_integration_ic.TIClient, ids []int64,
) error {
	var toFetch []int64
	for _, id := range ids {
		if _, ok := s.resolves[id]; !ok {
			toFetch = append(toFetch, id)
		}
	}
	if len(toFetch) == 0 {
		return nil
	}

	if s.resolves == nil {
		s.resolves = make(map[int64]team_integration_ic.ResolvedFolder)
	}

	for _, id := range toFetch {
		rsl, err := tiClient.ResolveABC(ctx, id)
		if err != nil {
			return err
		}
		s.resolves[rsl.ID] = rsl
	}
	return nil
}
