package iam

import (
	"context"
	"sync"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	iam_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/ctxtool"
)

func (s *Session) GetFolders(
	ctx context.Context, _ entities.ProcessingScope, ids []string,
) (result []entities.Folder, err error) {
	result = make([]entities.Folder, 0, len(ids))
	bs := s.paramsBatchSize()
	for i := 0; i < len(ids); i += bs {
		start := i
		end := i + bs
		if end > len(ids) {
			end = len(ids)
		}
		batchResult, err := s.adapter.folders.get(ctxtool.WithGlobalCancel(ctx, s.adapter.runCtx), s.adapter.rmClient, ids[start:end])
		if err != nil {
			return nil, err
		}
		result = append(result, batchResult...)
	}
	return
}

type foldersResolveStorage struct {
	mu sync.RWMutex

	fldToCld map[string]string
}

func (s *foldersResolveStorage) get(ctx context.Context, rmClient iam_ic.RMClient, ids []string) ([]entities.Folder, error) {
	var onReturn func()
	defer func() {
		if onReturn != nil {
			onReturn()
		}
	}()

	s.mu.RLock()
	onReturn = s.mu.RUnlock

	// if there are folders that missed in storage we should fetch them.
	if s.hasUnknownID(ids) {
		s.mu.RUnlock()
		onReturn = nil
		s.mu.Lock()
		onReturn = s.mu.Unlock

		// NOTE: some folders could be fetched while we wait for lock
		// so we pass all ids to fetch and filter them inside.
		if err := s.fetch(ctx, rmClient, ids); err != nil {
			return nil, err
		}
	}

	return s.resolve(ids), nil
}

func (s *foldersResolveStorage) hasUnknownID(ids []string) bool {
	for _, id := range ids {
		if _, ok := s.fldToCld[id]; !ok {
			return true
		}
	}
	return false
}

func (s *foldersResolveStorage) resolve(ids []string) (result []entities.Folder) {
	result = make([]entities.Folder, 0, len(ids))
	for _, id := range ids {
		result = append(result, entities.Folder{
			FolderID: id,
			CloudID:  s.fldToCld[id],
		})
	}
	return
}

func (s *foldersResolveStorage) fetch(ctx context.Context, rmClient iam_ic.RMClient, ids []string) error {
	var toFetch []string
	for _, id := range ids {
		if _, ok := s.fldToCld[id]; !ok {
			toFetch = append(toFetch, id)
		}
	}
	if len(toFetch) == 0 {
		return nil
	}

	resolve, err := rmClient.ResolveFolder(ctx, toFetch...)
	if err != nil {
		return err
	}

	if s.fldToCld == nil {
		s.fldToCld = make(map[string]string)
	}
	for _, r := range resolve {
		s.fldToCld[r.ID] = r.CloudID
	}
	return nil
}
