package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type ProductVersionsProvider interface {
	Get(ctx context.Context, params GetProductVersionsParams) ([]ProductVersion, error)

	status.StatusChecker
}

type GetProductVersionsParams struct {
	IDs []string
}

type ProductVersion struct {
	ID string `db:"id"`

	Payload      *ydb.AnyJSON `db:"payload"`
	LicenseRules *ydb.AnyJSON `db:"license_rules"`
}

type productVersionsProvider struct {
	*ydb.Connector
}

func NewProductVersionsProvider(c *ydb.Connector) ProductVersionsProvider {
	return &productVersionsProvider{
		Connector: c,
	}
}

func (p *productVersionsProvider) Get(ctx context.Context, params GetProductVersionsParams) ([]ProductVersion, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		%[1]s

		SELECT
			pv.id AS id,
			pv.license_rules AS license_rules,
			pv.payload as payload
		FROM
			%[2]s AS ids
		JOIN
			%[3]s AS pv
		ON
			ids.id = pv.id;`

		productVersionsTablePath = "meta/product_versions"
		idField                  = "id"
		declaredType             = "ids"
	)

	if len(params.IDs) == 0 {
		ctxtools.Logger(ctx).Debug("requested empty set of ids")
		return nil, nil
	}

	queryBuilder := ydb.NewQueryBuilder(p.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.DeclareType(declaredType, ydb.MakeListOfStringStructType(idField)),
		queryBuilder.TableExpression(declaredType),
		queryBuilder.TablePath(productVersionsTablePath),
	)

	var (
		ids    = ydb.MakeStringsList(idField, params.IDs)
		result []ProductVersion
	)

	err := p.DB().SelectContext(ctx, &result, query, sql.Named(declaredType, ids))
	if err := p.MapDBError(ctx, err); err != nil {
		return nil, err
	}

	return result, nil
}
