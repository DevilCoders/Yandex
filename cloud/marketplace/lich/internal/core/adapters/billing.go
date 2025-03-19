package adapters

import (
	"context"
	"fmt"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/structs"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
)

type BillingAdapter interface {
	GetBillingAccountByCloudID(ctx context.Context, cloudID string) (*model.BillingAccount, error)
	GetBillingAccountStructMap(ctx context.Context, cloudID string) (structs.Mapping, error)
}

// NOTE: concrete implementation should go to separate package.
type restBillingAdapterSession struct {
	sessionManager billing.SessionManager
	tokenProvider  auth.DefaultTokenAuth
}

func (r *restBillingAdapterSession) GetBillingAccountByCloudID(ctx context.Context, cloudID string) (*model.BillingAccount, error) {
	scopedLogger := ctxtools.LoggerWith(ctx, log.String("cloud_id", cloudID))

	scopedLogger.Debug("billing account retrieving has been started")

	iamToken, err := r.tokenProvider.Token(ctx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	result, err := r.sessionManager.SessionWithYCSubjectToken(ctx, iamToken).ResolveBillingAccounts(billing.ResolveBillingAccountsParams{
		CloudID: cloudID,
	})

	if err := r.mapError(ctx, err); err != nil {
		scopedLogger.Error("failed to request billing accounts", log.Error(err))
		return nil, err
	}

	if len(result) == 0 {
		// NOTE: Probably should be separate error type.
		scopedLogger.Error("empty result set")
		return nil, fmt.Errorf("unexpected number of accounts in response: %d %w", len(result), ErrNotFound)
	}

	scopedLogger.Debug("billing account retrieving completed", log.Int("count", len(result)))

	return makeBillingAccount(&result[0]), nil
}

func (r *restBillingAdapterSession) GetBillingAccountStructMap(ctx context.Context, cloudID string) (structs.Mapping, error) {
	span, spanCtx := tracing.Start(ctx, "billing:GetBillingAccountStructMap")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx, log.String("cloud_id", cloudID))

	scopedLogger.Debug("extended billing account retrieving has been started")

	iamToken, err := r.tokenProvider.Token(spanCtx)
	if err != nil {
		scopedLogger.Error("failed to get token", log.Error(err))
		return nil, err
	}

	now := time.Now().Unix()
	result, err := r.sessionManager.
		SessionWithYCSubjectToken(spanCtx, iamToken).
		ResolveBillingAccountByCloudIDFull(cloudID, now)

	if err := r.mapError(spanCtx, err); err != nil {
		scopedLogger.Error("failed to request billing accounts", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("extended billing account retrieving completed")

	return structs.NewMapping(result)
}

func NewBillingAdapter(sessionManager billing.SessionManager, tokenProvider auth.DefaultTokenAuth) BillingAdapter {
	return &restBillingAdapterSession{
		sessionManager: sessionManager,
		tokenProvider:  tokenProvider,
	}
}

func makeBillingAccount(in *billing.BillingAccount) *model.BillingAccount {
	return &model.BillingAccount{
		ID:   in.ID,
		Name: in.Name,

		UsageStatus: in.UsageStatus,
	}
}

func (r *restBillingAdapterSession) mapError(ctx context.Context, err error) error {
	if err == nil {
		ctxtools.Logger(ctx).Debug("billing adapter: completed without error")
		return nil
	}

	scoppedLogger := ctxtools.LoggerWith(ctx, log.Error(err))
	scoppedLogger.Error("billing client request failed with error")

	if xerrors.Is(err, context.Canceled) || xerrors.Is(err, context.DeadlineExceeded) {
		scoppedLogger.Error("context cancelled or deadline exceeded", log.Error(err))
		return err
	}

	switch {
	case xerrors.Is(err, billing.ErrInternalClientError):
		return fmt.Errorf("failed to connect to billing service: %w", ErrInternalServerError)
	case xerrors.Is(err, billing.ErrInternalServerError):
		return fmt.Errorf("billing service internal error: %w", ErrInternalServerError)
	case xerrors.Is(err, billing.ErrNotAuthenticated):
		return fmt.Errorf("failed to authenticate: %w", ErrNotAuthenticated)
	case xerrors.Is(err, billing.ErrNotAuthorized):
		return fmt.Errorf("not authorized: %w", ErrNotAuthorized)
	case xerrors.Is(err, billing.ErrNotFound):
		return fmt.Errorf("not found: %w", ErrNotFound)
	case xerrors.Is(err, billing.ErrBackendTimeout):
		return fmt.Errorf("billing internal error: %w", ErrBackendTimeout)
	default:
		scoppedLogger.Error("unclassified error")
		return err
	}
}
