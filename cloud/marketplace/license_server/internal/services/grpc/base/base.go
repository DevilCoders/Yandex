package base

import (
	"context"
	"errors"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"

	"google.golang.org/grpc/status"

	errors_actions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	errors_ydb_adapter "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	errors_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/library/go/core/log"
)

type Service struct {
	*env.Env
}

func (b *Service) MapActionError(ctx context.Context, err error) error {
	if err == nil {
		logging.Logger().Debug("action completed without errors")
		return err
	}

	logging.Logger().Error("action failed", log.Error(err))

	var (
		statusErr interface {
			GRPCStatus() *status.Status
		}
	)

	switch {
	case errors.Is(err, errors_ydb_adapter.ErrNotFoundLicenseTemplate):
		return errors_grpc.MakeErrNotFound(err.Error())
	case errors.Is(err, errors_ydb_adapter.ErrNotFoundLicenseTemplateVersion):
		return errors_grpc.MakeErrNotFound(err.Error())
	case errors.Is(err, errors_ydb_adapter.ErrNotFoundLicenseInstance):
		return errors_grpc.MakeErrNotFound(err.Error())
	case errors.Is(err, errors_ydb_adapter.ErrNotFoundLicenseLock):
		return errors_grpc.MakeErrNotFound(err.Error())
	case errors.Is(err, errors_ydb_adapter.ErrNotFoundOperation):
		return errors_grpc.MakeErrNotFound(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseTemplateVersionIsAlreadyExists):
		return errors_grpc.MakeErrAlreadyExists(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseInstanceAlreadyLocked):
		return errors_grpc.MakeErrAlreadyExists(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseInstanceNotActive):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseTemplateNotPendingAndActive):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseTemplateVersionNotPending):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.Is(err, errors_actions.ErrLicenseTemplateVersionNotDeprecated):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.Is(err, model.ErrInvalidPeriod):
		return errors_grpc.MakeErrInvalidArgument(err.Error())
	case errors.Is(err, model.ErrInvalidValue):
		return errors_grpc.MakeErrInvalidArgument(err.Error())
	case errors.Is(err, model.ErrInvalidMeasure):
		return errors_grpc.MakeErrInvalidArgument(err.Error())
	case errors.Is(err, errors_actions.ErrTariffNotActiveOrPending):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.Is(err, errors_actions.ErrTariffNotPAYG):
		return errors_grpc.MakeErrFailedPrecondition(err.Error())
	case errors.As(err, &statusErr):
		logging.Logger().Error("general grpc error", log.Error(err))
		return statusErr.GRPCStatus().Err()
	default:
		logging.Logger().Error("unclassified internal error", log.Error(err))
		return errors_grpc.GRPCInternalErr
	}
}
