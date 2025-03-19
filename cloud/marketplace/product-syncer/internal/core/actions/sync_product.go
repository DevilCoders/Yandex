package actions

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/services/env"
	"a.yandex-team.ru/library/go/core/log"
)

type SyncProductAction struct {
	*env.Env
}

func NewSyncProductAction(env *env.Env) *SyncProductAction {
	return &SyncProductAction{env}
}

func (a *SyncProductAction) Do(ctx context.Context, params model.ProductYaml) error {
	span, spanCtx := tracing.Start(ctx, "SyncProductAction")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx,
		log.String("product_name", params.ProductName),
	)

	scopedLogger.Debug("syncing product")
	op, err := a.Adapters().Marketplace().SyncProduct(spanCtx, marketplace.SyncProductParams{
		ID:          params.ProductID,
		PublisherID: params.PublisherID,
		Type:        params.ProductType,
		Name:        params.ProductName,
	})
	if err != nil {
		return err
	}
	scopedLogger.Debug("sync product: ", log.String("Operation", op.ID))
	return nil
}
