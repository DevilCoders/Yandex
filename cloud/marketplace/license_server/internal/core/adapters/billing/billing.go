package billing

import (
	"context"
	"errors"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/core/log"
)

type BillingAdapter interface {
	CreateSku(ctx context.Context, params CreateSkuParams) (*model.Sku, error)
}

type restBillingAdapterSession struct {
	sessionManager billing.SessionManager
	tokenProvider  auth.DefaultTokenAuth
}

func NewBillingAdapter(sessionManager billing.SessionManager, tokenProvider auth.DefaultTokenAuth) BillingAdapter {
	return &restBillingAdapterSession{
		sessionManager: sessionManager,
		tokenProvider:  tokenProvider,
	}
}

type CreateSkuParams struct{}

func (r *restBillingAdapterSession) CreateSku(ctx context.Context, params CreateSkuParams) (*model.Sku, error) {
	scopedLogger := ctxtools.LoggerWith(ctx, log.Any("create_sku_params", params))

	scopedLogger.Debug("create sku has been started")

	iamToken, err := r.tokenProvider.Token(ctx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(ctx, iamToken).CreateSku(billing.CreateSkuParams{})

	if err := r.mapError(ctx, err); err != nil {
		scopedLogger.Error("failed to create sku", log.Error(err))
		return nil, err
	}

	return makeSku(result), nil
}

func makeSku(in billing.Sku) *model.Sku {
	return &model.Sku{
		ID: in.ID,
	}
}

func (r *restBillingAdapterSession) mapError(ctx context.Context, err error) error {
	if err == nil {
		ctxtools.Logger(ctx).Debug("billing adapter: completed without error")
		return nil
	}

	scoppedLogger := ctxtools.LoggerWith(ctx, log.Error(err))
	scoppedLogger.Error("billing client request failed with error")

	if errors.Is(err, context.Canceled) || errors.Is(err, context.DeadlineExceeded) {
		scoppedLogger.Error("context cancelled or deadline exceeded", log.Error(err))
		return err
	}

	switch {
	case errors.Is(err, billing.ErrInternalClientError):
		return fmt.Errorf("failed to connect to billing service: %w", adapters.ErrInternalServerError)
	case errors.Is(err, billing.ErrInternalServerError):
		return fmt.Errorf("billing service internal error: %w", adapters.ErrInternalServerError)
	case errors.Is(err, billing.ErrNotAuthenticated):
		return fmt.Errorf("failed to authenticate: %w", adapters.ErrNotAuthenticated)
	case errors.Is(err, billing.ErrNotAuthorized):
		return fmt.Errorf("not authorized: %w", adapters.ErrNotAuthorized)
	case errors.Is(err, billing.ErrNotFound):
		return fmt.Errorf("not found: %w", adapters.ErrNotFound)
	case errors.Is(err, billing.ErrBackendTimeout):
		return fmt.Errorf("billing internal error: %w", adapters.ErrBackendTimeout)
	default:
		scoppedLogger.Error("unclassified error")
		return err
	}
}

// TODO: implement create sku and send metric
