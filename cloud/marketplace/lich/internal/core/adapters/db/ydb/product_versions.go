package ydb

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"
)

type productVersionAdapter struct {
	backend ydb.ProductVersionsProvider
}

func NewProductVersionAdapter(backend ydb.ProductVersionsProvider) *productVersionAdapter {
	return &productVersionAdapter{
		backend: backend,
	}
}

func (p *productVersionAdapter) GetProductVersions(ctx context.Context, ids ...string) ([]model.ProductVersion, error) {
	span, spanCtx := tracing.Start(ctx, "db:GetProductVersions")
	defer span.Finish()

	scopedLogger := logging.LoggerWith(log.Strings("ids", ids))

	scopedLogger.Debug("product-version-adapter: executing get product versions")

	dbResult, err := p.backend.Get(spanCtx, ydb.GetProductVersionsParams{
		IDs: ids,
	})

	if err != nil {
		scopedLogger.Error("product-version-adapter: database request failed", log.Error(err))
		return nil, err
	}

	var result []model.ProductVersion
	for i := range dbResult {
		version, err := productVersionFromYDB(&dbResult[i])
		if err != nil {
			scopedLogger.Error("product-version-adapter: failed to convert product version from db format", log.Error(err))
			return nil, err
		}

		result = append(result, *version)
	}

	scopedLogger.Debug("product-version-adapter: execution of get product versions has been completed", log.Int("count", len(result)))

	return result, nil
}

func (p *productVersionAdapter) Ping(ctx context.Context) error {
	return p.backend.Ping(ctx)
}
