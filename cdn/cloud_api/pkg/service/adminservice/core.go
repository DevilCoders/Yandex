package adminservice

import (
	"context"
	"fmt"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cdn/cloud_api/pkg/configuration"
	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
)

const pageSize = 50

type Service interface {
	RunDatabaseGC() error
}

type ServiceImpl struct {
	Logger           log.Logger
	AdminStorage     storage.AdminStorage
	Storage          storage.Storage
	DatabaseGCConfig configuration.DatabaseGCConfig
}

// TODO: cron
func (s *ServiceImpl) RunDatabaseGC() error {
	return s.runDatabaseGC(context.Background())
}

func (s *ServiceImpl) runDatabaseGC(ctx context.Context) error {
	s.Logger.Info("database gc: run")
	startTime := time.Now()

	if s.DatabaseGCConfig.EraseSoftDeleted.Enabled {
		err := s.eraseSoftDeleted(ctx, s.DatabaseGCConfig.EraseSoftDeleted.TimeThreshold)
		if err != nil {
			return err
		}
	}

	if s.DatabaseGCConfig.EraseOldVersions.Enabled {
		err := s.eraseOldVersions(ctx, s.DatabaseGCConfig.EraseOldVersions.VersionThreshold)
		if err != nil {
			return err
		}
	}

	elapsedTime := time.Since(startTime)
	s.Logger.Infof("database gc: finish, elapsed time: %s", elapsedTime)

	return nil
}

func (s *ServiceImpl) eraseSoftDeleted(ctx context.Context, threshold time.Duration) error {
	timeThreshold := time.Now().Add(-threshold)

	group, ctx := errgroup.WithContext(ctx)

	group.Go(func() error {
		errorResult := s.AdminStorage.EraseSoftDeletedResource(ctx, timeThreshold)
		if errorResult != nil {
			return fmt.Errorf("erase soft deleted resource: %w", errorResult.Error())
		}
		return nil
	})

	group.Go(func() error {
		errorResult := s.AdminStorage.EraseSoftDeletedOriginsGroup(ctx, timeThreshold)
		if errorResult != nil {
			return fmt.Errorf("ererase soft deleted origins group: %w", errorResult.Error())
		}
		return nil
	})

	return group.Wait()
}

func (s *ServiceImpl) eraseOldVersions(ctx context.Context, threshold int64) error {
	group, ctx := errgroup.WithContext(ctx)

	group.Go(func() error {
		errorResult := s.eraseOldResourceVersions(ctx, threshold)
		if errorResult != nil {
			return fmt.Errorf("erase old resource versions: %w", errorResult.Error())
		}
		return nil
	})

	group.Go(func() error {
		errorResult := s.eraseOldOriginsGroupVersions(ctx, threshold)
		if errorResult != nil {
			return fmt.Errorf("erase old origins group versions: %w", errorResult.Error())
		}
		return nil
	})

	return group.Wait()
}

func (s *ServiceImpl) eraseOldResourceVersions(ctx context.Context, threshold int64) errors.ErrorResult {
	page := &model.Pagination{
		PageToken: 1,
		PageSize:  pageSize,
	}

	for {
		select {
		case <-ctx.Done():
			return errors.WrapError("ctx done", errors.InternalError, ctx.Err())
		default:
		}

		resources, errorResult := s.Storage.GetAllResources(ctx, makePage(page))
		if errorResult != nil {
			return errorResult
		}
		if len(resources) == 0 {
			break
		}

		for _, resource := range resources {
			version := int64(resource.EntityVersion) - threshold
			if version <= 0 {
				continue
			}

			errorResult = s.AdminStorage.EraseOldResourceVersions(ctx, resource.EntityID, version)
			if errorResult != nil {
				return errorResult
			}
		}

		if len(resources) < pageSize {
			break
		}
		page.PageToken++
	}

	return nil
}

func (s *ServiceImpl) eraseOldOriginsGroupVersions(ctx context.Context, threshold int64) errors.ErrorResult {
	page := &model.Pagination{
		PageToken: 1,
		PageSize:  pageSize,
	}

	for {
		select {
		case <-ctx.Done():
			return errors.WrapError("ctx done", errors.InternalError, ctx.Err())
		default:
		}

		originsGroups, errorResult := s.Storage.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
			FolderID:       nil,
			GroupIDs:       nil,
			PreloadOrigins: false,
			Page:           makePage(page),
		})
		if errorResult != nil {
			return errorResult
		}
		if len(originsGroups) == 0 {
			break
		}

		for _, group := range originsGroups {
			version := int64(group.EntityVersion) - threshold
			if version <= 0 {
				continue
			}

			errorResult = s.AdminStorage.EraseOldOriginsGroupVersions(ctx, group.EntityID, version)
			if errorResult != nil {
				return errorResult
			}
		}

		if len(originsGroups) < pageSize {
			break
		}
		page.PageToken++
	}

	return nil
}

func makePage(page *model.Pagination) *storage.Pagination {
	if page == nil {
		return nil
	}

	return &storage.Pagination{
		Offset: page.Offset(),
		Limit:  page.Limit(),
	}
}
