package meta

import (
	"context"
	"database/sql"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type skuTestSuite struct {
	baseSuite

	skus []SkuRow
	sts  []SchemaToSkuRow
	pts  []ProductToSkuRow
}

func TestSku(t *testing.T) {
	suite.Run(t, new(skuTestSuite))
}

func (suite *skuTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
	suite.skus = suite.skus[:0]
	suite.sts = suite.sts[:0]
	suite.pts = suite.pts[:0]
}

func (suite *skuTestSuite) TestSkuCompliance() {
	err := suite.queries.pushSkus(suite.ctx, SkuRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllSkus(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *skuTestSuite) TestSchemaToSkuCompliance() {
	err := suite.queries.pushSchemaToSkus(suite.ctx, SchemaToSkuRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllSchemaToSkus(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *skuTestSuite) TestProductToSkuCompliance() {
	err := suite.queries.pushProductToSkus(suite.ctx, ProductToSkuRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllProductToSkus(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *skuTestSuite) TestGetResolving() {
	suite.pushData()

	wantSku := []SkuInfoRow{
		{ID: "sku1", Name: "sku1", PricingUnit: "pu1", Formula: "usage.quantity", UsageUnit: "uu1", UsageType: "delta"},
		{ID: "sku2", Name: "sku2", PricingUnit: "pu2", Formula: "usage.quantity", UsageUnit: "uu2", UsageType: "cumulative"},
		{ID: "explicit", Name: "explicit", PricingUnit: "puE", Formula: "usage.quantity", UsageUnit: "uuE", UsageType: "cumulative"},
	}

	wantScm := []SkuBySchemaResolveRow{
		{SkuID: "sku1", Schema: "scm1", ResolvingRules: qtool.JSONAnything("null"), ResolvingPolicy: "policy"},
		{SkuID: "sku1", Schema: "scm1.1", ResolvingRules: qtool.JSONAnything("null"), ResolvingPolicy: "policy"},
		{SkuID: "sku2", Schema: "scm2", ResolvingRules: qtool.JSONAnything("null"), ResolvingPolicy: "policy"},
		{SkuID: "deprecated", Schema: "deprecated", ResolvingRules: qtool.JSONAnything("null"), ResolvingPolicy: ""},
	}

	wantPrd := []SkuByProductResolveRow{
		{SkuID: "sku1", ProductID: "prd1", ResolvingRules: qtool.JSONAnything("null"), CheckFormula: "formula"},
		{SkuID: "sku1", ProductID: "prd1.1", ResolvingRules: qtool.JSONAnything("null"), CheckFormula: "formula"},
		{SkuID: "sku2", ProductID: "prd2", ResolvingRules: qtool.JSONAnything("null"), CheckFormula: "formula"},
		{SkuID: "deprecated", ProductID: "deprecated", ResolvingRules: qtool.JSONAnything("null"), CheckFormula: "formula"},
		{SkuID: "unmatched", ProductID: "unmatched", ResolvingRules: qtool.JSONAnything("null"), CheckFormula: "formula"},
	}

	gotScm, gotPrd, gotSku, err := suite.queries.GetSkuResolvingBySchema(suite.ctx,
		[]string{
			"scm1",
			"scm1.1",
			"scm2",
			"deprecated",
			"unmatched",
		},
		[]string{
			"scm1",
			"scm1.1",
			"scm2",
			"deprecated",
			"unmatched",
		},
		[]string{"explicit"},
	)
	suite.Require().NoError(err)
	suite.ElementsMatch(wantScm, gotScm)
	suite.ElementsMatch(wantPrd, gotPrd)
	suite.ElementsMatch(wantSku, gotSku)
}

func (suite *skuTestSuite) TestGetResolvingEmptyScm() {
	suite.pushData()

	gotScm, gotPrd, _, err := suite.queries.GetSkuResolvingBySchema(suite.ctx,
		[]string{},
		[]string{
			"scm1",
			"scm1.1",
			"scm2",
			"deprecated",
			"unmatched",
		},
		[]string{},
	)
	suite.Require().NoError(err)
	suite.Empty(gotScm)
	suite.NotEmpty(gotPrd)
}

func (suite *skuTestSuite) TestGetResolvingEmptyPrd() {
	suite.pushData()

	gotScm, gotPrd, _, err := suite.queries.GetSkuResolvingBySchema(suite.ctx,
		[]string{
			"scm1",
			"scm1.1",
			"scm2",
			"deprecated",
			"unmatched",
		},
		[]string{},
		[]string{},
	)
	suite.Require().NoError(err)
	suite.NotEmpty(gotScm)
	suite.Empty(gotPrd)
}

func (suite *skuTestSuite) pushData() {
	suite.skus = []SkuRow{
		{ID: "sku1", Name: "sku1", PricingUnit: "pu1", Formula: "usage.quantity", UsageUnit: "uu1", ResolvingPolicy: "policy", UsageType: "delta"},
		{ID: "sku2", Name: "sku2", PricingUnit: "pu2", Formula: "usage.quantity", UsageUnit: "uu2", ResolvingPolicy: "policy", UsageType: "cumulative"},
		{ID: "explicit", Name: "explicit", PricingUnit: "puE", Formula: "usage.quantity", UsageUnit: "uuE", ResolvingPolicy: "policy", UsageType: "cumulative"},
		{ID: "unused", Name: "unused"},
		{ID: "deprecated", Name: "deprecated", Deprecated: true},
	}
	err := suite.queries.pushSkus(suite.ctx, suite.skus...)
	suite.Require().NoError(err)

	suite.sts = []SchemaToSkuRow{
		{Schema: "scm1", SkuID: "sku1"},
		{Schema: "scm1.1", SkuID: "sku1"},
		{Schema: "scm2", SkuID: "sku2"},
		{Schema: "deprecated", SkuID: "deprecated"},
		{Schema: "unmatched", SkuID: "unmatched"},
		{Schema: "unused", SkuID: "unused"},
	}
	err = suite.queries.pushSchemaToSkus(suite.ctx, suite.sts...)
	suite.Require().NoError(err)

	suite.pts = []ProductToSkuRow{
		{ProductID: "prd1", SkuID: "sku1", CheckFormula: "formula"},
		{ProductID: "prd1.1", SkuID: "sku1", CheckFormula: "formula"},
		{ProductID: "prd2", SkuID: "sku2", CheckFormula: "formula"},
		{ProductID: "deprecated", SkuID: "deprecated", CheckFormula: "formula"},
		{ProductID: "unused", SkuID: "unused", CheckFormula: "formula"},
		{ProductID: "unmatched", SkuID: "unmatched", CheckFormula: "formula"},
	}
	err = suite.queries.pushProductToSkus(suite.ctx, suite.pts...)
	suite.Require().NoError(err)
}

func (q *Queries) pushSkus(ctx context.Context, rows ...SkuRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushSkuQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) getAllSkus(ctx context.Context) (result []SkuRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllSkusQuery.WithParams(q.qp))
	return
}

func (q *Queries) pushSchemaToSkus(ctx context.Context, rows ...SchemaToSkuRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushSchemaToSkuQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) getAllSchemaToSkus(ctx context.Context) (result []SchemaToSkuRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllSchemaToSkuQuery.WithParams(q.qp))
	return
}

func (q *Queries) pushProductToSkus(ctx context.Context, rows ...ProductToSkuRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushProductToSkuQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) getAllProductToSkus(ctx context.Context) (result []ProductToSkuRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllProductToSkuQuery.WithParams(q.qp))
	return
}

type SkuRow struct {
	MetricUnit         string             `db:"metric_unit"`
	RateFormula        string             `db:"rate_formula"`
	PricingVersions    qtool.JSONAnything `db:"pricing_versions"`
	ResolvingPolicy    string             `db:"resolving_policy"`
	PricingUnit        string             `db:"pricing_unit"`
	UsageUnit          string             `db:"usage_unit"`
	ID                 string             `db:"id"`
	Priority           uint64             `db:"priority"`
	BalanceProductID   string             `db:"balance_product_id"`
	Formula            string             `db:"formula"`
	ServiceID          string             `db:"service_id"`
	Name               string             `db:"name"`
	CreatedAt          qtool.UInt64Ts     `db:"created_at"`
	PublisherAccountID string             `db:"publisher_account_id"`
	Translations       qtool.JSONAnything `db:"translations"`
	UpdatedAt          qtool.UInt64Ts     `db:"updated_at"`
	UpdatedBy          string             `db:"updated_by"`
	UsageType          string             `db:"usage_type"`
	ResolvingRules     qtool.JSONAnything `db:"resolving_rules"`
	Deprecated         bool               `db:"deprecated"`
}

func (r SkuRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("metric_unit", ydb.UTF8Value(r.MetricUnit)),
		ydb.StructFieldValue("rate_formula", ydb.UTF8Value(r.RateFormula)),
		ydb.StructFieldValue("pricing_versions", r.PricingVersions.Value()),
		ydb.StructFieldValue("resolving_policy", ydb.UTF8Value(r.ResolvingPolicy)),
		ydb.StructFieldValue("pricing_unit", ydb.UTF8Value(r.PricingUnit)),
		ydb.StructFieldValue("usage_unit", ydb.UTF8Value(r.UsageUnit)),
		ydb.StructFieldValue("id", ydb.UTF8Value(r.ID)),
		ydb.StructFieldValue("priority", ydb.Uint64Value(r.Priority)),
		ydb.StructFieldValue("balance_product_id", ydb.UTF8Value(r.BalanceProductID)),
		ydb.StructFieldValue("formula", ydb.UTF8Value(r.Formula)),
		ydb.StructFieldValue("service_id", ydb.UTF8Value(r.ServiceID)),
		ydb.StructFieldValue("name", ydb.UTF8Value(r.Name)),
		ydb.StructFieldValue("created_at", r.CreatedAt.Value()),
		ydb.StructFieldValue("publisher_account_id", ydb.UTF8Value(r.PublisherAccountID)),
		ydb.StructFieldValue("translations", r.Translations.Value()),
		ydb.StructFieldValue("updated_at", r.UpdatedAt.Value()),
		ydb.StructFieldValue("updated_by", ydb.UTF8Value(r.UpdatedBy)),
		ydb.StructFieldValue("usage_type", ydb.UTF8Value(r.UsageType)),
		ydb.StructFieldValue("resolving_rules", r.ResolvingRules.Value()),
		ydb.StructFieldValue("deprecated", ydb.BoolValue(r.Deprecated)),
	)
}

var (
	skuStructType = ydb.Struct(
		ydb.StructField("metric_unit", ydb.TypeUTF8),
		ydb.StructField("rate_formula", ydb.TypeUTF8),
		ydb.StructField("pricing_versions", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("resolving_policy", ydb.TypeUTF8),
		ydb.StructField("pricing_unit", ydb.TypeUTF8),
		ydb.StructField("usage_unit", ydb.TypeUTF8),
		ydb.StructField("id", ydb.TypeUTF8),
		ydb.StructField("priority", ydb.TypeUint64),
		ydb.StructField("balance_product_id", ydb.TypeUTF8),
		ydb.StructField("formula", ydb.TypeUTF8),
		ydb.StructField("service_id", ydb.TypeUTF8),
		ydb.StructField("name", ydb.TypeUTF8),
		ydb.StructField("created_at", ydb.TypeUint64),
		ydb.StructField("publisher_account_id", ydb.TypeUTF8),
		ydb.StructField("translations", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("updated_at", ydb.TypeUint64),
		ydb.StructField("updated_by", ydb.TypeUTF8),
		ydb.StructField("usage_type", ydb.TypeUTF8),
		ydb.StructField("resolving_rules", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("deprecated", ydb.TypeBool),
	)
	skuListType = ydb.List(skuStructType)

	pushSkuQuery = qtool.Query(
		qtool.Declare("values", skuListType),
		qtool.ReplaceFromValues("meta/skus",
			"metric_unit",
			"rate_formula",
			"pricing_versions",
			"resolving_policy",
			"pricing_unit",
			"usage_unit",
			"id",
			"priority",
			"balance_product_id",
			"formula",
			"service_id",
			"name",
			"created_at",
			"publisher_account_id",
			"translations",
			"updated_at",
			"updated_by",
			"usage_type",
			"resolving_rules",
			"deprecated",
		),
		qtool.ReplaceFromValues("meta/skus_name_id_idx",
			"id",
			"name",
		),
		qtool.ReplaceFromValues("meta/skus_usage_type_id_idx",
			"id",
			"usage_type",
		),
		qtool.ReplaceFromValues("meta/skus_service_id_id_idx",
			"id",
			"service_id",
		),
	)

	getAllSkusQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("meta/skus"),
	)
)

type SchemaToSkuRow struct {
	Schema string `db:"schema"`
	SkuID  string `db:"sku_id"`
}

func (r SchemaToSkuRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("schema", ydb.UTF8Value(r.Schema)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
	)
}

var (
	schemaToSkuStructType = ydb.Struct(
		ydb.StructField("schema", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
	)
	schemaToSkuListType = ydb.List(schemaToSkuStructType)

	pushSchemaToSkuQuery = qtool.Query(
		qtool.Declare("values", schemaToSkuListType),
		qtool.ReplaceFromValues("meta/schema_to_skus",
			"schema",
			"sku_id",
		),
		qtool.ReplaceFromValues("meta/schema_to_skus_sku_id_idx",
			"schema",
			"sku_id",
		),
	)

	getAllSchemaToSkuQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("meta/schema_to_skus"),
	)
)

type ProductToSkuRow struct {
	ProductID      string             `db:"product_id"`
	SkuID          string             `db:"sku_id"`
	Hash           uint64             `db:"hash"`
	CheckFormula   string             `db:"check_formula"`
	ResolvingRules qtool.JSONAnything `db:"resolving_rules"`
}

func (r ProductToSkuRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("product_id", ydb.UTF8Value(r.ProductID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("hash", ydb.Uint64Value(r.Hash)),
		ydb.StructFieldValue("check_formula", ydb.UTF8Value(r.CheckFormula)),
		ydb.StructFieldValue("resolving_rules", r.ResolvingRules.Value()),
	)
}

var (
	productToSkuStructType = ydb.Struct(
		ydb.StructField("product_id", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("hash", ydb.TypeUint64),
		ydb.StructField("check_formula", ydb.TypeUTF8),
		ydb.StructField("resolving_rules", ydb.Optional(ydb.TypeJSON)),
	)
	productToSkuListType = ydb.List(productToSkuStructType)

	pushProductToSkuQuery = qtool.Query(
		qtool.Declare("values", productToSkuListType),
		qtool.ReplaceFromValues("meta/product_to_skus_v2",
			"product_id",
			"sku_id",
			"hash",
			"check_formula",
			"resolving_rules",
		),
		qtool.ReplaceFromValues("meta/product_to_skus_v2_sku_id_idx",
			"product_id",
			"sku_id",
			"hash",
		),
	)

	getAllProductToSkuQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("meta/product_to_skus_v2"),
	)
)
