package actions

import (
	"context"
	"strings"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/mds/go/xstrings"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/permissions"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
)

type LicenseCheckParams struct {
	CloudID     string
	ProductsIDs []string
}

type LicenseCheckAction struct {
	baseAction
}

type LicenseCheckResult struct {
	LicensedInstancePool string
}

func NewLicenseCheckAction(env *env.Env) *LicenseCheckAction {
	return &LicenseCheckAction{
		baseAction: baseAction{env},
	}
}

func (a *LicenseCheckAction) Do(ctx context.Context, params LicenseCheckParams) (*LicenseCheckResult, error) {
	span, spanCtx := tracing.Start(ctx, "LicenseCheckAction")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx,
		log.String("cloud_id", params.CloudID),
		log.Strings("product_versions", params.ProductsIDs),
	)

	scopedLogger.Info("executing license check action")

	spanCtx = ctxtools.WithTag(spanCtx, "cloud_id", params.CloudID)
	spanCtx = ctxtools.WithTag(spanCtx, "product_ids", params.ProductsIDs...)

	if err := a.checkParams(spanCtx, params); err != nil {
		return nil, err
	}

	if err := a.Auth().AuthorizeBillingAdmin(spanCtx, permissions.LicenseCheckPermission); err != nil {
		return nil, err
	}

	var (
		cloudPermissionsStages []string
	)

	group, requestContext := errgroup.WithContext(spanCtx)

	group.Go(func() error {
		scopedLogger.Debug("requesting resource manager: get_permission_stages")

		var err error
		cloudPermissionsStages, err = a.Backends().ResourceManager().GetPermissionStages(requestContext, params.CloudID)

		return a.mapRMError(spanCtx, params.CloudID, err)
	})

	var productVersions []model.ProductVersion
	group.Go(func() error {
		scopedLogger.Debug("requesting database: get_product_version")

		var err error
		productVersions, err = a.Adapters().DB().GetProductVersions(requestContext, params.ProductsIDs...)

		return err
	})

	if err := group.Wait(); err != nil {
		return nil, err
	}

	var (
		licensedInstancePool = xstrings.NewSet()
		rulesSpec            []model.RuleSpec
	)

	for _, version := range productVersions {
		if version.Payload.ComputeImage != nil && version.Payload.ComputeImage.ResourceSpec.LicenseInstancePool != "" {
			licensedInstancePool.Add(
				version.Payload.ComputeImage.ResourceSpec.LicenseInstancePool,
			)
		}

		rulesSpec = append(rulesSpec, version.LicenseRules...)
	}

	permissionStages := model.NewPermissionStages(cloudPermissionsStages...)

	if len(rulesSpec) == 0 {
		scopedLogger.Info("empty rules spec listing for product version")
	}

	if len(rulesSpec) > 0 {
		billingAccountMapping, err := a.Adapters().Billing().GetBillingAccountStructMap(spanCtx, params.CloudID)
		if err := a.mapBillingError(spanCtx, params.CloudID, err); err != nil {
			return nil, err
		}

		all, err := model.MakeAllLicenseRulesExpression(billingAccountMapping, &permissionStages, rulesSpec...)
		if err != nil {
			return nil, err
		}

		if ok, externalMessages := all.Evaluate(); !ok {
			return nil, newErrLicenseCheckExternal(
				params.CloudID,
				params.ProductsIDs,
				externalMessages,
			)
		}
	}

	if len(licensedInstancePool) > 1 {
		return nil, ErrLicensedInstancePoolValue
	}

	return a.makePlacementHintResult(params.CloudID, licensedInstancePool, &permissionStages), nil
}

func (a *LicenseCheckAction) checkParams(ctx context.Context, params LicenseCheckParams) error {
	if params.CloudID == "" {
		return ErrNoCloudID
	}

	if len(params.ProductsIDs) == 0 {
		return ErrEmptyProductIDs
	}

	ctxtools.Logger(ctx).Debug("versions id passed to license check", log.Strings("product_ids", params.ProductsIDs))

	return nil
}

func (a *LicenseCheckAction) makePlacementHintResult(
	cloudID string, licenseInstancePool xstrings.StringsSet, permissionStages *model.PermissionStages,
) *LicenseCheckResult {

	var emptyResult = &LicenseCheckResult{}

	if len(licenseInstancePool) < 1 {
		return emptyResult
	}

	const (
		first = iota
	)

	placementHint := licenseInstancePool.Elements()[first]

	if placementHint == "" {
		return emptyResult
	}

	normalizedPlacementHint := strings.ReplaceAll(placementHint, "-", "_")
	flag := "DISABLE_PLACEMENT_HINT_" + normalizedPlacementHint

	if permissionStages.Member(flag) {
		logging.Logger().Info("placement hint disabled for cloud",
			log.String("cloud_id", cloudID),
			log.String("flag", flag),
			log.String("placement_hint", placementHint),
		)

		return emptyResult
	}

	result := &LicenseCheckResult{
		LicensedInstancePool: placementHint,
	}

	return result
}
