package adapters

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/library/go/core/log"
)

type MarketplaceAdapter interface {
	GetCategoryByID(ctx context.Context, categoryID string) (*marketplace.Category, error)
	GetProductByID(ctx context.Context, publisherID, productID string) (*marketplace.Product, error)
	GetPublisherByID(ctx context.Context, productID string) (*marketplace.Publisher, error)
	GetVersionByID(ctx context.Context, publisherID, productID, versionID string) (*marketplace.Version, error)
	SyncProduct(ctx context.Context, params marketplace.SyncProductParams) (*marketplace.Operation, error)
	SyncVersion(ctx context.Context, params marketplace.SyncVersionParams) (*marketplace.Operation, error)
	GetFreeProductTariff(ctx context.Context, publisherID, productID string) (*string, error)
}

type restMarketplaceAdapterSession struct {
	sessionManager marketplace.SessionManager
	tokenProvider  auth.DefaultTokenAuth
}

func (r *restMarketplaceAdapterSession) GetCategoryByID(ctx context.Context, categoryID string) (*marketplace.Category, error) {
	span, spanCtx := tracing.Start(ctx, "GetCategoryByID")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("category_id", categoryID))

	scopedLogger.Debug("category retrieving has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).GetCategoryByIDorName(categoryID)

	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to request category", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("category retrieving completed")

	return result, nil
}

func (r *restMarketplaceAdapterSession) GetProductByID(ctx context.Context, publisherID, productID string) (*marketplace.Product, error) {
	span, spanCtx := tracing.Start(ctx, "GetProductByID")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("product_id", productID))

	scopedLogger.Debug("product retrieving has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).GetProductByID(marketplace.GetProductParams{
		ProductID:   productID,
		PublisherID: publisherID,
	})

	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to request product", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("product retrieving completed")

	return result, nil
}

func (r *restMarketplaceAdapterSession) GetPublisherByID(ctx context.Context, publisherID string) (*marketplace.Publisher, error) {
	span, spanCtx := tracing.Start(ctx, "GetPublisherByID")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("publisher_id", publisherID))

	scopedLogger.Debug("publisher retrieving has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).GetPublisherByID(publisherID)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to request publisher", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("publisher retrieving completed")
	return result, nil
}

func (r *restMarketplaceAdapterSession) GetVersionByID(ctx context.Context, publisherID, productID, versionID string) (*marketplace.Version, error) {
	span, spanCtx := tracing.Start(ctx, "GetVersionByID")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("version_id", versionID))

	scopedLogger.Debug("version retrieving has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).GetVersionByID(marketplace.GetVersionParams{
		ProductID:   productID,
		PublisherID: publisherID,
		VersionID:   versionID,
	})
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to request version", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("version retrieving completed")
	return result, nil
}

func (r *restMarketplaceAdapterSession) SyncProduct(ctx context.Context, params marketplace.SyncProductParams) (*marketplace.Operation, error) {
	span, spanCtx := tracing.Start(ctx, "SyncProduct")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("product_id", params.ID))

	scopedLogger.Debug("sync product has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	op, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).SyncProduct(params)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to sync product", log.Error(err))
		return nil, err
	}

	op, err = r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).WaitOperation(op.ID)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("operation waiting failed", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("sync product completed")
	return op, nil
}

func (r *restMarketplaceAdapterSession) SyncVersion(ctx context.Context, params marketplace.SyncVersionParams) (*marketplace.Operation, error) {
	span, spanCtx := tracing.Start(ctx, "SyncVersion")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("version_id", params.ID))

	scopedLogger.Debug("sync version has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	op, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).SyncVersion(params)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to sync version", log.Error(err))
		return nil, err
	}

	op, err = r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).WaitOperation(op.ID)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("operation waiting failed", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("sync version completed")
	return op, nil
}

func (r *restMarketplaceAdapterSession) GetFreeProductTariff(ctx context.Context, publisherID, productID string) (*string, error) {
	span, spanCtx := tracing.Start(ctx, "GetFreeTariff")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("product_id", productID))
	scopedLogger.Debug("get free tariff has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	tariffs, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).ListTariffsByProductID(publisherID, productID)
	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to list tariffs", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("listed tariffs")
	if len(tariffs) > 0 {
		return &tariffs[0].ID, nil
	} else {
		scopedLogger.Debug("need to create free tariff")
		// TODO: remove sleep. But without it we get weird read: connection reset by peer errors from python app
		time.Sleep(time.Second * 3)

		tariffOp, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).CreateTariff(marketplace.CreateTariffParams{
			ProductID:   productID,
			PublisherID: publisherID,
			Name:        "Free",
			Definition:  marketplace.FreeDefinition{},
		})
		if err := r.mapError(spanCtx, err); err != nil {
			scopedLogger.Error("failed to create tariff", log.Error(err))
			return nil, err
		}

		op, err := r.sessionManager.SessionWithYCSubjectToken(spanCtx, iamToken).WaitOperation(tariffOp.ID)
		if err := r.mapError(spanCtx, err); err != nil {
			scopedLogger.Error("operation waiting failed", log.Error(err))
			return nil, err
		}

		var tariff marketplace.Tariff
		err = json.Unmarshal(op.Response, &tariff)
		if err := r.mapError(spanCtx, err); err != nil {
			scopedLogger.Error("failed to parse tariff operation response", log.Error(err))
			return nil, err
		}

		return &tariff.ID, err
	}
}

func NewMarketplaceAdapter(sessionManager marketplace.SessionManager, tokenProvider auth.DefaultTokenAuth) MarketplaceAdapter {
	return &restMarketplaceAdapterSession{
		sessionManager: sessionManager,
		tokenProvider:  tokenProvider,
	}
}

func (r *restMarketplaceAdapterSession) mapError(ctx context.Context, err error) error {
	if err == nil {
		return nil
	}

	scoppedLogger := ctxtools.LoggerWith(ctx, log.Error(err))
	scoppedLogger.Error("marketplace client request failed with error")

	if xerrors.Is(err, context.Canceled) || xerrors.Is(err, context.DeadlineExceeded) {
		scoppedLogger.Error("context cancelled or deadline exceeded")
		return err
	}

	switch {
	case xerrors.Is(err, marketplace.ErrInternalClientError):
		return fmt.Errorf("failed to connect to marketplace service: %w", ErrInternalServerError)
	case xerrors.Is(err, marketplace.ErrInternalServerError):
		return fmt.Errorf("marketplace service internal error: %w", ErrInternalServerError)
	case xerrors.Is(err, marketplace.ErrNotAuthenticated):
		return fmt.Errorf("failed to authenticate: %w", ErrNotAuthenticated)
	case xerrors.Is(err, marketplace.ErrNotAuthorized):
		return fmt.Errorf("not authorized: %w", ErrNotAuthorized)
	case xerrors.Is(err, marketplace.ErrBadRequest):
		return fmt.Errorf("bad request: %w", err)
	case xerrors.Is(err, marketplace.ErrNotFound):
		return marketplace.ErrNotFound
	default:
		scoppedLogger.Error("unclassified error")
		return err
	}
}
