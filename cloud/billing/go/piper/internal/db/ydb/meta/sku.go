package meta

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
)

func (q *Queries) GetSkuResolvingBySchema(ctx context.Context, schemas []string, productSchemas []string, explicitSkus []string) (
	schResolve []SkuBySchemaResolveRow, prdResolve []SkuByProductResolveRow, skus []SkuInfoRow, err error,
) {
	ctx = tooling.QueryStarted(ctx)
	query := skuResolvingQuery.WithParams(q.qp)
	defer func() {
		err = qtool.WrapWithQuery(err, query)
		tooling.QueryDone(ctx, err)
	}()

	scmParam := makeStringList(notEmptyStrings(schemas))
	prdParam := makeStringList(notEmptyStrings(productSchemas))
	expParam := makeStringList(notEmptyStrings(explicitSkus))

	rows, err := q.db.QueryxContext(ctx, query,
		sql.Named("schemas", scmParam),
		sql.Named("prd_schemas", prdParam),
		sql.Named("explicit", expParam),
	)
	if err != nil {
		return
	}
	defer func() {
		_ = rows.Close()
	}()

	for rows.Next() {
		var row SkuBySchemaResolveRow
		if err = rows.StructScan(&row); err != nil {
			return
		}
		schResolve = append(schResolve, row)
	}
	if err = rows.Err(); err != nil {
		return
	}

	if rows, err = qtool.NextResultSet(rows); err != nil {
		return
	}
	for rows.Next() {
		var row SkuByProductResolveRow
		if err = rows.StructScan(&row); err != nil {
			return
		}
		prdResolve = append(prdResolve, row)
	}
	if err = rows.Err(); err != nil {
		return
	}

	if rows, err = qtool.NextResultSet(rows); err != nil {
		return
	}
	for rows.Next() {
		var row SkuInfoRow
		if err = rows.StructScan(&row); err != nil {
			return
		}
		skus = append(skus, row)
	}
	if err = rows.Err(); err != nil {
		return
	}
	// trace count of skus returned by query
	tooling.QueryWithRowsCount(ctx, len(skus))
	return
}

type SkuInfoRow struct {
	ID          string `db:"id"`
	Name        string `db:"name"`
	PricingUnit string `db:"pricing_unit"`
	Formula     string `db:"formula"`
	UsageUnit   string `db:"usage_unit"`
	UsageType   string `db:"usage_type"`
}

type SkuBySchemaResolveRow struct {
	SkuID           string             `db:"id"`
	Schema          string             `db:"schema"`
	ResolvingRules  qtool.JSONAnything `db:"resolving_rules"`
	ResolvingPolicy qtool.String       `db:"resolving_policy"`
}

type SkuByProductResolveRow struct {
	SkuID          string             `db:"id"`
	ProductID      string             `db:"product_id"`
	ResolvingRules qtool.JSONAnything `db:"resolving_rules"`
	CheckFormula   qtool.String       `db:"check_formula"`
}

var skuResolvingQuery = qtool.Query(
	qtool.Declare("schemas", stringList),
	qtool.Declare("prd_schemas", stringList),
	qtool.Declare("explicit", stringList),

	qtool.NamedQuery("scm_skus",
		"SELECT", qtool.PrefixedCols("scm", "sku_id", "schema"),
		"FROM", qtool.TableAs("meta/schema_to_skus", "scm"),
		"WHERE scm.schema in $schemas",
	),
	qtool.NamedQuery("prd_schemas_ids",
		"SELECT", qtool.PrefixedCols("sku_idx", "sku_id"),
		"FROM", qtool.TableAs("meta/schema_to_skus", "sku_idx"),
		"WHERE sku_idx.schema in $prd_schemas",
	),
	qtool.NamedQuery("product_ids",
		"SELECT DISTINCT", qtool.PrefixedCols("prod_idx", "product_id"),
		"FROM $prd_schemas_ids as sku_idx",
		" JOIN", qtool.TableAs("meta/product_to_skus_v2_sku_id_idx", "prod_idx"), "ON sku_idx.sku_id = prod_idx.sku_id",
	),
	qtool.NamedQuery("relevant_skus",
		"SELECT", qtool.PrefixedCols("skus", "id", "resolving_rules", "resolving_policy"),
		"FROM", qtool.TableAs("meta/skus", "skus"),
		"WHERE id IN (SELECT sku_id FROM $scm_skus)",
	),
	qtool.NamedQuery("scm_resolves",
		"SELECT", qtool.Cols(
			qtool.PrefixedCols("skus", "id", "resolving_rules", "resolving_policy"),
			qtool.PrefixedCols("scm", "schema"),
		),
		"FROM $relevant_skus as skus",
		" JOIN $scm_skus as scm ON skus.id = scm.sku_id",
		"WHERE (skus.resolving_rules IS NOT NULL or skus.resolving_policy IS NOT NULL)",
	),
	qtool.NamedQuery("prd_resolves",
		"SELECT", qtool.Cols(
			"products.sku_id as id",
			qtool.PrefixedCols("products", "product_id", "resolving_rules", "check_formula"),
		),
		"FROM $product_ids as prod_idx",
		" JOIN", qtool.TableAs("meta/product_to_skus_v2", "products"), "ON products.product_id = prod_idx.product_id",
		"WHERE (products.resolving_rules is NOT NULL or products.check_formula is NOT NULL)",
	),
	qtool.NamedQuery("sku_ids",
		"SELECT DISTINCT id FROM(",
		" SELECT id FROM $scm_resolves",
		" UNION ALL",
		" SELECT id FROM $prd_resolves",
		" UNION ALL",
		" SELECT id FROM", qtool.TableAs("meta/skus", "skus"),
		" WHERE id in $explicit",
		")",
	),

	"SELECT", qtool.Cols("id", "schema", "resolving_rules", "resolving_policy"), "FROM $scm_resolves;",

	"SELECT", qtool.Cols("id", "product_id", "resolving_rules", "check_formula"), "FROM $prd_resolves;",

	"SELECT", qtool.PrefixedCols("skus", "id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"),
	"FROM $sku_ids as idx",
	" JOIN", qtool.TableAs("meta/skus", "skus"), "ON idx.id = skus.id",
	"WHERE usage_type IS NOT NULL AND NOT NVL(skus.deprecated, false);",
)
