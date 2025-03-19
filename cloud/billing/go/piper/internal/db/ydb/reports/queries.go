package reports

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type Queries interface {
	PushResourceReports(ctx context.Context, updatedAt qtool.UInt64Ts, rows ...ResourceReportsRow) (err error)
}

type DBTX interface {
	ExecContext(ctx context.Context, query string, args ...interface{}) (sql.Result, error)
	SelectContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
}

type queries struct {
	db DBTX
	qp qtool.QueryParams
}

func (q *queries) PushResourceReports(ctx context.Context, updatedAt qtool.UInt64Ts, rows ...ResourceReportsRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	query := pushResourceReportsQuery.WithParams(q.qp)
	_, err = q.db.ExecContext(ctx, query,
		sql.Named("updated_at", updatedAt.Value()),
		sql.Named("records", param.List()),
	)
	return
}

var (
	pushResourceReportsQuery = qtool.Query(
		qtool.Declare("records", resourceReportsListType),
		qtool.Declare("updated_at", ydb.TypeUint64),
		qtool.DecimalZeroDecl,
		"$nvl = ($amount) -> {RETURN NVL($amount, $zero)};",

		"UPSERT INTO", qtool.Table("reports/realtime/resource_reports"),
		"SELECT", qtool.Cols(
			qtool.PrefixedCols("a",
				"billing_account_id",
				"cloud_id",
				"`date`",
				"folder_id",
				"labels_hash",
				"resource_id",
				"sku_id",
			),
			"$updated_at as `updated_at`",
			"a.`cost` + $nvl(b.`cost`) AS `cost`",
			"a.`credit` + $nvl(b.`credit`) AS `credit`",
			"a.`cud_credit` + $nvl(b.`cud_credit`) AS `cud_credit`",
			"a.`free_credit` + $nvl(b.`free_credit`) AS `free_credit`",
			"a.`pricing_quantity` + $nvl(b.`pricing_quantity`) AS `pricing_quantity`",
			"a.`monetary_grant_credit` + $nvl(b.`monetary_grant_credit`) AS `monetary_grant_credit`",
			"a.`volume_incentive_credit` + $nvl(b.`volume_incentive_credit`) AS `volume_incentive_credit`",
		),
		"FROM as_table($records) AS a",
		"LEFT JOIN", qtool.TableAs("reports/realtime/resource_reports", "b"),
		"USING (`billing_account_id`,`date`,`cloud_id`,`folder_id`,`sku_id`,`resource_id`,`labels_hash`);",
	)

	resourceReportsStruct = ydb.Struct(
		ydb.StructField("billing_account_id", ydb.TypeUTF8),
		ydb.StructField("`date`", ydb.TypeUTF8),
		ydb.StructField("cloud_id", ydb.TypeUTF8),
		ydb.StructField("sku_id", ydb.TypeUTF8),
		ydb.StructField("folder_id", ydb.TypeUTF8),
		ydb.StructField("resource_id", ydb.TypeUTF8),
		ydb.StructField("labels_hash", ydb.TypeUint64),
		ydb.StructField("pricing_quantity", ydb.Decimal(22, 9)),
		ydb.StructField("cost", ydb.Decimal(22, 9)),
		ydb.StructField("credit", ydb.Decimal(22, 9)),
		ydb.StructField("cud_credit", ydb.Decimal(22, 9)),
		ydb.StructField("monetary_grant_credit", ydb.Decimal(22, 9)),
		ydb.StructField("volume_incentive_credit", ydb.Decimal(22, 9)),
		ydb.StructField("free_credit", ydb.Decimal(22, 9)),
	)
	resourceReportsListType = ydb.List(resourceReportsStruct)
)

func NewQueries(db DBTX, params qtool.QueryParams) Queries {
	return &queries{db: db, qp: params}
}

type ResourceReportsRowKey struct {
	BillingAccountID string
	Date             qtool.DateString
	CloudID          string
	SkuID            string
	FolderID         string
	ResourceID       string
	LabelsHash       uint64
}

type ResourceReportsRow struct {
	BillingAccountID      string               `db:"billing_account_id"`
	Date                  qtool.DateString     `db:"date"`
	CloudID               string               `db:"cloud_id"`
	SkuID                 string               `db:"sku_id"`
	FolderID              string               `db:"folder_id"`
	ResourceID            string               `db:"resource_id"`
	LabelsHash            uint64               `db:"labels_hash"`
	PricingQuantity       qtool.DefaultDecimal `db:"pricing_quantity"`
	Cost                  qtool.DefaultDecimal `db:"cost"`
	Credit                qtool.DefaultDecimal `db:"credit"`
	CudCredit             qtool.DefaultDecimal `db:"cud_credit"`
	MonetaryGrantCredit   qtool.DefaultDecimal `db:"monetary_grant_credit"`
	VolumeIncentiveCredit qtool.DefaultDecimal `db:"volume_incentive_credit"`
	FreeCredit            qtool.DefaultDecimal `db:"free_credit"`
	UpdatedAt             qtool.UInt64Ts       `db:"updated_at"`
}

func (r ResourceReportsRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("billing_account_id", ydb.UTF8Value(r.BillingAccountID)),
		ydb.StructFieldValue("date", r.Date.Value()),
		ydb.StructFieldValue("cloud_id", ydb.UTF8Value(r.CloudID)),
		ydb.StructFieldValue("sku_id", ydb.UTF8Value(r.SkuID)),
		ydb.StructFieldValue("folder_id", ydb.UTF8Value(r.FolderID)),
		ydb.StructFieldValue("resource_id", ydb.UTF8Value(r.ResourceID)),
		ydb.StructFieldValue("labels_hash", ydb.Uint64Value(r.LabelsHash)),
		ydb.StructFieldValue("pricing_quantity", r.PricingQuantity.Value()),
		ydb.StructFieldValue("cost", r.Cost.Value()),
		ydb.StructFieldValue("credit", r.Credit.Value()),
		ydb.StructFieldValue("cud_credit", r.CudCredit.Value()),
		ydb.StructFieldValue("monetary_grant_credit", r.MonetaryGrantCredit.Value()),
		ydb.StructFieldValue("volume_incentive_credit", r.VolumeIncentiveCredit.Value()),
		ydb.StructFieldValue("free_credit", r.FreeCredit.Value()),
	)
}

func (r ResourceReportsRow) YDBKeyTuple() ydb.Value {
	return ydb.TupleValue(
		ydb.UTF8Value(r.BillingAccountID),
		r.Date.Value(),
		ydb.UTF8Value(r.CloudID),
		ydb.UTF8Value(r.FolderID),
		ydb.UTF8Value(r.SkuID),
		ydb.UTF8Value(r.ResourceID),
		ydb.Uint64Value(r.LabelsHash),
	)
}
