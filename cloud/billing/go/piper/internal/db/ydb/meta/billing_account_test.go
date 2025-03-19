package meta

import (
	"context"
	"database/sql"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type billingAccountsTestSuite struct {
	baseSuite
}

func TestBillingAccounts(t *testing.T) {
	suite.Run(t, new(billingAccountsTestSuite))
}

func (suite *billingAccountsTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
}

func (suite *billingAccountsTestSuite) TestBATableCompliance() {
	err := suite.queries.pushBA(suite.ctx, BillingAccountRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllBA(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *billingAccountsTestSuite) TestGetMapping() {
	err := suite.queries.pushBA(suite.ctx,
		[]BillingAccountRow{
			{ID: "lonelyBA"},
			{ID: "ba1_1", MasterAccountID: "acc1"},
			{ID: "ba1_2", MasterAccountID: "acc1"},
			{ID: "ba2", MasterAccountID: "acc2"},
		}...,
	)
	suite.Require().NoError(err)

	got, err := suite.queries.GetBAMappingByID(suite.ctx,
		"lonelyBA",
		"ba1_1",
		"ba1_2",
	)
	suite.Require().NoError(err)
	wantMap := []BillingAccountMappingRow{
		{ID: "lonelyBA"},
		{ID: "ba1_1", MasterAccountID: "acc1"},
		{ID: "ba1_2", MasterAccountID: "acc1"},
	}
	suite.ElementsMatch(wantMap, got)
}

func (suite *billingAccountsTestSuite) TestBindingsTableCompliance() {
	err := suite.queries.pushInstanceBindings(suite.ctx, InstanceBindingTableRow{})
	suite.Require().NoError(err)

	_, err = suite.queries.getAllInstanceBindings(suite.ctx)
	suite.Require().NoError(err)
}

func (suite *billingAccountsTestSuite) TestGetInstanceBindings() {
	err := suite.queries.pushInstanceBindings(suite.ctx, []InstanceBindingTableRow{
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc1", EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc2", EffectiveTime: qtool.UInt64Ts(time.Unix(1000, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc3", EffectiveTime: qtool.UInt64Ts(time.Unix(2000, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc99", EffectiveTime: qtool.UInt64Ts(time.Unix(3000, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld2", BillingAccountID: "acc1", EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld3", BillingAccountID: "acc1", EffectiveTime: qtool.UInt64Ts(time.Unix(9999, 0))},
		{ServiceInstanceType: "cloud", ServiceInstanceID: "other", BillingAccountID: "acc99", EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0))},
		{ServiceInstanceType: "tracker", ServiceInstanceID: "tr", BillingAccountID: "acc99", EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0))},
	}...)
	suite.Require().NoError(err)

	got, err := suite.queries.GetBABindings(suite.ctx, "cloud", time.Unix(1000, 0), time.Unix(2000, 0), "cld1", "cld2", "cld3")
	suite.Require().NoError(err)

	want := []InstanceBindingRow{
		{
			ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc2",
			EffectiveTime: qtool.UInt64Ts(time.Unix(1000, 0)), EffectiveTo: qtool.UInt64Ts(time.Unix(2000, 0)),
		},
		{
			ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc3",
			EffectiveTime: qtool.UInt64Ts(time.Unix(2000, 0)), EffectiveTo: qtool.UInt64Ts(time.Unix(3000, 0)),
		},
		{
			ServiceInstanceType: "cloud", ServiceInstanceID: "cld2", BillingAccountID: "acc1",
			EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0)), EffectiveTo: qtool.UInt64Ts(qtool.InfTime),
		},
	}
	suite.ElementsMatch(want, got)
}

func (suite *billingAccountsTestSuite) TestGetInstanceBindingsWithUnbinds() {
	err := suite.queries.pushInstanceBindings(suite.ctx, []InstanceBindingTableRow{
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc1", EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0))},
	}...)
	suite.Require().NoError(err)
	err = suite.queries.pushInstanceBindingsWithoutBA(suite.ctx, []InstanceBindingTableRow{
		{ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", EffectiveTime: qtool.UInt64Ts(time.Unix(1500, 0))},
	}...)
	suite.Require().NoError(err)

	got, err := suite.queries.GetBABindings(suite.ctx, "cloud", time.Unix(1000, 0), time.Unix(2000, 0), "cld1")
	suite.Require().NoError(err)

	want := []InstanceBindingRow{
		{
			ServiceInstanceType: "cloud", ServiceInstanceID: "cld1", BillingAccountID: "acc1",
			EffectiveTime: qtool.UInt64Ts(time.Unix(0, 0)), EffectiveTo: qtool.UInt64Ts(time.Unix(1500, 0)),
		},
	}
	suite.ElementsMatch(want, got)
}

func (q *Queries) pushBA(ctx context.Context, rows ...BillingAccountRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushBAQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) getAllBA(ctx context.Context) (result []BillingAccountRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllBAQuery.WithParams(q.qp))
	return
}

func (q *Queries) pushInstanceBindings(ctx context.Context, rows ...InstanceBindingTableRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushInstanceBindingQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) pushInstanceBindingsWithoutBA(ctx context.Context, rows ...InstanceBindingTableRow) (err error) {
	param := qtool.ListValues()
	for _, r := range rows {
		param.Add(r)
	}
	_, err = q.db.ExecContext(ctx, pushInstanceBindingWithoutBAQuery.WithParams(q.qp), sql.Named("values", param.List()))
	return
}

func (q *Queries) getAllInstanceBindings(ctx context.Context) (result []InstanceBindingTableRow, err error) {
	err = q.db.SelectContext(ctx, &result, getAllInstanceBindingsQuery.WithParams(q.qp))
	return
}

type BillingAccountRow struct {
	UpdatedAt         qtool.UInt64Ts       `db:"updated_at"`
	Balance           qtool.DefaultDecimal `db:"balance"`
	BalanceClientID   qtool.String         `db:"balance_client_id"`
	ID                string               `db:"id"`
	PaymentMethodID   qtool.String         `db:"payment_method_id"`
	Currency          string               `db:"currency"`
	PersonID          string               `db:"person_id"`
	PaymentCycleType  string               `db:"payment_cycle_type"`
	CreatedAt         qtool.UInt64Ts       `db:"created_at"`
	State             string               `db:"state"`
	PersonType        string               `db:"person_type"`
	Metadata          qtool.JSONAnything   `db:"metadata"`
	MasterAccountID   qtool.String         `db:"master_account_id"`
	Type              string               `db:"type"`
	PaymentType       string               `db:"payment_type"`
	FeatureFlags      qtool.JSONAnything   `db:"feature_flags"`
	OwnerID           string               `db:"owner_id"`
	BalanceContractID string               `db:"balance_contract_id"`
	UsageStatus       string               `db:"usage_status"`
	ClientID          string               `db:"client_id"`
	BillingThreshold  qtool.DefaultDecimal `db:"billing_threshold"`
	Name              string               `db:"name"`
	CountryCode       string               `db:"country_code"`
	PaidAt            qtool.UInt64Ts       `db:"paid_at"`
	DisabledAt        qtool.UInt64Ts       `db:"disabled_at"`
}

func (r BillingAccountRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("updated_at", r.UpdatedAt.Value()),
		ydb.StructFieldValue("balance", r.Balance.Value()),
		ydb.StructFieldValue("balance_client_id", r.BalanceClientID.Value()),
		ydb.StructFieldValue("id", ydb.UTF8Value(r.ID)),
		ydb.StructFieldValue("payment_method_id", r.PaymentMethodID.Value()),
		ydb.StructFieldValue("currency", ydb.UTF8Value(r.Currency)),
		ydb.StructFieldValue("person_id", ydb.UTF8Value(r.PersonID)),
		ydb.StructFieldValue("payment_cycle_type", ydb.UTF8Value(r.PaymentCycleType)),
		ydb.StructFieldValue("created_at", r.CreatedAt.Value()),
		ydb.StructFieldValue("state", ydb.UTF8Value(r.State)),
		ydb.StructFieldValue("person_type", ydb.UTF8Value(r.PersonType)),
		ydb.StructFieldValue("metadata", r.Metadata.Value()),
		ydb.StructFieldValue("master_account_id", r.MasterAccountID.Value()),
		ydb.StructFieldValue("type", ydb.UTF8Value(r.Type)),
		ydb.StructFieldValue("payment_type", ydb.UTF8Value(r.PaymentType)),
		ydb.StructFieldValue("feature_flags", r.FeatureFlags.Value()),
		ydb.StructFieldValue("owner_id", ydb.UTF8Value(r.OwnerID)),
		ydb.StructFieldValue("balance_contract_id", ydb.UTF8Value(r.BalanceContractID)),
		ydb.StructFieldValue("usage_status", ydb.UTF8Value(r.UsageStatus)),
		ydb.StructFieldValue("client_id", ydb.UTF8Value(r.ClientID)),
		ydb.StructFieldValue("billing_threshold", r.BillingThreshold.Value()),
		ydb.StructFieldValue("name", ydb.UTF8Value(r.Name)),
		ydb.StructFieldValue("country_code", ydb.UTF8Value(r.CountryCode)),
		ydb.StructFieldValue("paid_at", r.PaidAt.Value()),
		ydb.StructFieldValue("disabled_at", r.DisabledAt.Value()),
	)
}

var (
	baStructType = ydb.Struct(
		ydb.StructField("updated_at", ydb.TypeUint64),
		ydb.StructField("balance", ydb.Decimal(22, 9)),
		ydb.StructField("balance_client_id", ydb.Optional(ydb.TypeUTF8)),
		ydb.StructField("id", ydb.TypeUTF8),
		ydb.StructField("payment_method_id", ydb.Optional(ydb.TypeUTF8)),
		ydb.StructField("currency", ydb.TypeUTF8),
		ydb.StructField("person_id", ydb.TypeUTF8),
		ydb.StructField("payment_cycle_type", ydb.TypeUTF8),
		ydb.StructField("created_at", ydb.TypeUint64),
		ydb.StructField("state", ydb.TypeUTF8),
		ydb.StructField("person_type", ydb.TypeUTF8),
		ydb.StructField("metadata", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("master_account_id", ydb.Optional(ydb.TypeUTF8)),
		ydb.StructField("type", ydb.TypeUTF8),
		ydb.StructField("payment_type", ydb.TypeUTF8),
		ydb.StructField("feature_flags", ydb.Optional(ydb.TypeJSON)),
		ydb.StructField("owner_id", ydb.TypeUTF8),
		ydb.StructField("balance_contract_id", ydb.TypeUTF8),
		ydb.StructField("usage_status", ydb.TypeUTF8),
		ydb.StructField("client_id", ydb.TypeUTF8),
		ydb.StructField("billing_threshold", ydb.Decimal(22, 9)),
		ydb.StructField("name", ydb.TypeUTF8),
		ydb.StructField("country_code", ydb.TypeUTF8),
		ydb.StructField("paid_at", ydb.TypeUint64),
		ydb.StructField("disabled_at", ydb.TypeUint64),
	)
	baListType = ydb.List(baStructType)

	pushBAQuery = qtool.Query(
		qtool.Declare("values", baListType),
		qtool.ReplaceFromValues("meta/billing_accounts",
			"updated_at",
			"balance",
			"balance_client_id",
			"id",
			"payment_method_id",
			"currency",
			"person_id",
			"payment_cycle_type",
			"created_at",
			"state",
			"person_type",
			"metadata",
			"master_account_id",
			"type",
			"payment_type",
			"feature_flags",
			"owner_id",
			"balance_contract_id",
			"usage_status",
			"client_id",
			"billing_threshold",
			"name",
			"country_code",
			"paid_at",
			"disabled_at",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_master_account_id_idx",
			"id",
			"master_account_id",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_owner_id_idx",
			"id",
			"owner_id",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_balance_client_id_idx",
			"id",
			"balance_client_id",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_client_id_idx",
			"id",
			"client_id",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_balance_contract_id_idx",
			"id",
			"balance_contract_id",
		),
		qtool.ReplaceFromValues("meta/billing_accounts_state_idx",
			"id",
			"state",
		),
	)

	getAllBAQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("meta/billing_accounts"),
	)
)

type InstanceBindingTableRow struct {
	ServiceInstanceType string          `db:"service_instance_type"`
	ServiceInstanceID   string          `db:"service_instance_id"`
	BillingAccountID    string          `db:"billing_account_id"`
	EffectiveTime       qtool.UInt64Ts  `db:"effective_time"`
	CreatedAt           ydbsql.Datetime `db:"created_at"`
}

func (r InstanceBindingTableRow) YDBStruct() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("service_instance_type", ydb.UTF8Value(r.ServiceInstanceType)),
		ydb.StructFieldValue("service_instance_id", ydb.UTF8Value(r.ServiceInstanceID)),
		ydb.StructFieldValue("billing_account_id", ydb.UTF8Value(r.BillingAccountID)),
		ydb.StructFieldValue("effective_time", r.EffectiveTime.Value()),
		ydb.StructFieldValue("created_at", r.CreatedAt.Value()),
	)
}

var (
	instanceBindingTableType = ydb.Struct(
		ydb.StructField("service_instance_type", ydb.TypeUTF8),
		ydb.StructField("service_instance_id", ydb.TypeUTF8),
		ydb.StructField("billing_account_id", ydb.TypeUTF8),
		ydb.StructField("effective_time", ydb.TypeUint64),
		ydb.StructField("created_at", ydb.TypeDatetime),
	)
	instanceBindingTableListType = ydb.List(instanceBindingTableType)

	pushInstanceBindingQuery = qtool.Query(
		qtool.Declare("values", instanceBindingTableListType),
		qtool.ReplaceFromValues("meta/service_instance_bindings/bindings",
			"service_instance_type",
			"service_instance_id",
			"billing_account_id",
			"effective_time",
			"created_at",
		),
		qtool.ReplaceFromValues("meta/service_instance_bindings/bindings_billing_account_id_idx",
			"service_instance_type",
			"service_instance_id",
			"billing_account_id",
			"effective_time",
		),
	)

	pushInstanceBindingWithoutBAQuery = qtool.Query(
		qtool.Declare("values", instanceBindingTableListType),
		qtool.ReplaceFromValues("meta/service_instance_bindings/bindings",
			"service_instance_type",
			"service_instance_id",
			"effective_time",
			"created_at",
		),

		"REPLACE INTO ", qtool.Table("meta/service_instance_bindings/bindings"),
		"(service_instance_type, service_instance_id, billing_account_id, effective_time)",
		"SELECT service_instance_type, service_instance_id, NULL as billing_account_id, effective_time",
		"FROM AS_TABLE($values);",
	)

	getAllInstanceBindingsQuery = qtool.Query(
		"SELECT * FROM", qtool.Table("meta/service_instance_bindings/bindings"),
	)
)
