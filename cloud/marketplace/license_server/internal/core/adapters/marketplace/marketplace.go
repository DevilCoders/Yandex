package marketplace

import (
	"context"
	"errors"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/core/log"
)

type MarketplaceAdapter interface {
	GetTariff(ctx context.Context, params GetTariffParams) (*model.Tariff, error)
}

type restMarketplaceAdapterSession struct {
	sessionManager marketplace.SessionManager
	tokenProvider  auth.DefaultTokenAuth
}

func NewMarketplaceAdapter(sessionManager marketplace.SessionManager, tokenProvider auth.DefaultTokenAuth) MarketplaceAdapter {
	return &restMarketplaceAdapterSession{
		sessionManager: sessionManager,
		tokenProvider:  tokenProvider,
	}
}

type GetTariffParams struct {
	PublisherID string
	ProductID   string
	TariffID    string
}

func (r *restMarketplaceAdapterSession) GetTariff(ctx context.Context, params GetTariffParams) (*model.Tariff, error) {
	scopedLogger := ctxtools.LoggerWith(ctx, log.Any("get_taiff_params", params))

	scopedLogger.Debug("get tariff has been started")

	iamToken, err := r.tokenProvider.Token(ctx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(ctx, iamToken).GetTariff(marketplace.GetTariffParams{
		PublisherID: params.PublisherID,
		ProductID:   params.ProductID,
		TariffID:    params.TariffID,
	})

	if err := r.mapError(ctx, err); err != nil {
		scopedLogger.Error("failed to get tariff", log.Error(err))
		return nil, err
	}

	return makeTariff(result), nil
}

func makeTariff(in *marketplace.Tariff) *model.Tariff {
	return &model.Tariff{
		ID:          in.ID,
		ProductID:   in.ProductID,
		PublisherID: in.PublisherID,
		Type:        in.Type,
		Name:        in.Name,
		State:       in.State,
	}
}

func (r *restMarketplaceAdapterSession) mapError(ctx context.Context, err error) error {
	if err == nil {
		ctxtools.Logger(ctx).Debug("marketplace adapter: completed without error")
		return nil
	}

	scoppedLogger := ctxtools.LoggerWith(ctx, log.Error(err))
	scoppedLogger.Error("marketplace client request failed with error")

	if errors.Is(err, context.Canceled) || errors.Is(err, context.DeadlineExceeded) {
		scoppedLogger.Error("context cancelled or deadline exceeded", log.Error(err))
		return err
	}

	switch {
	case errors.Is(err, marketplace.ErrInternalClientError):
		return fmt.Errorf("failed to connect to billing service: %w", adapters.ErrInternalServerError)
	case errors.Is(err, marketplace.ErrInternalServerError):
		return fmt.Errorf("marketplace service internal error: %w", adapters.ErrInternalServerError)
	case errors.Is(err, marketplace.ErrNotAuthenticated):
		return fmt.Errorf("failed to authenticate: %w", adapters.ErrNotAuthenticated)
	case errors.Is(err, marketplace.ErrNotAuthorized):
		return fmt.Errorf("not authorized: %w", adapters.ErrNotAuthorized)
	case errors.Is(err, marketplace.ErrNotFound):
		return fmt.Errorf("not found: %w", adapters.ErrNotFound)
	default:
		scoppedLogger.Error("unclassified error")
		return err
	}
}
