package reports

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type resourceReportsTestSuite struct {
	baseSuite
}

func TestResourceReports(t *testing.T) {
	suite.Run(t, new(resourceReportsTestSuite))
}

func (suite *resourceReportsTestSuite) SetupTest() {
	suite.baseSuite.SetupTest()
}

func (suite *resourceReportsTestSuite) TearDownTest() {
	defer suite.baseSuite.TearDownTest()
}

func (suite *resourceReportsTestSuite) TestInsertResourceReports() {
	rows := suite.pushData()
	suite.Require().NotEmpty(rows)

	want := make([]ResourceReportsRow, 0)
	for _, r := range rows {
		want = append(want, ResourceReportsRow{
			BillingAccountID:      r.BillingAccountID,
			Date:                  r.Date,
			CloudID:               r.CloudID,
			SkuID:                 r.SkuID,
			FolderID:              r.FolderID,
			ResourceID:            r.ResourceID,
			LabelsHash:            r.LabelsHash,
			PricingQuantity:       r.PricingQuantity,
			Cost:                  r.Cost,
			Credit:                r.Credit,
			CudCredit:             r.CudCredit,
			MonetaryGrantCredit:   r.MonetaryGrantCredit,
			VolumeIncentiveCredit: r.VolumeIncentiveCredit,
			FreeCredit:            r.FreeCredit,
			UpdatedAt:             r.UpdatedAt,
		})
	}

	got, err := suite.testQueries.getResourceReports(suite.ctx)
	suite.Require().NoError(err)
	suite.ElementsMatch(want, got)
}

func (suite *resourceReportsTestSuite) TestUpdateResourceReports() {
	rows1 := suite.pushData()
	rows2 := suite.pushData()
	suite.Require().NotEmpty(rows1)
	suite.Require().NotEmpty(rows2)

	want := make([]ResourceReportsRow, 0)
	for _, r := range rows1 {
		want = append(want, ResourceReportsRow{
			BillingAccountID:      r.BillingAccountID,
			Date:                  r.Date,
			CloudID:               r.CloudID,
			SkuID:                 r.SkuID,
			FolderID:              r.FolderID,
			ResourceID:            r.ResourceID,
			LabelsHash:            r.LabelsHash,
			PricingQuantity:       decMul(r.PricingQuantity, "2"),
			Cost:                  decMul(r.Cost, "2"),
			Credit:                decMul(r.Credit, "2"),
			CudCredit:             decMul(r.CudCredit, "2"),
			MonetaryGrantCredit:   decMul(r.MonetaryGrantCredit, "2"),
			VolumeIncentiveCredit: decMul(r.VolumeIncentiveCredit, "2"),
			FreeCredit:            decMul(r.FreeCredit, "2"),
			UpdatedAt:             r.UpdatedAt,
		})
	}

	got, err := suite.testQueries.getResourceReports(suite.ctx)
	suite.Require().NoError(err)
	suite.ElementsMatch(want, got)
}

func (suite *resourceReportsTestSuite) pushData() []ResourceReportsRow {
	rows := []ResourceReportsRow{
		{
			BillingAccountID:      "baacc1",
			Date:                  qtool.DateString(time.Date(2000, 1, 1, 0, 0, 0, 0, time.Local)),
			CloudID:               "cloud1",
			FolderID:              "folder1",
			SkuID:                 "some-sku",
			ResourceID:            "res1",
			LabelsHash:            123,
			PricingQuantity:       dec("3"),
			Cost:                  dec("11.4"),
			Credit:                dec("11"),
			CudCredit:             dec("10"),
			FreeCredit:            dec("9"),
			MonetaryGrantCredit:   dec("8"),
			VolumeIncentiveCredit: dec("7"),
			UpdatedAt:             qtool.UInt64Ts(time.Unix(1, 0)),
		},
		{
			BillingAccountID:      "baacc1",
			Date:                  qtool.DateString(time.Date(2000, 1, 2, 0, 0, 0, 0, time.Local)),
			CloudID:               "cloud1",
			FolderID:              "folder1",
			SkuID:                 "some-sku",
			ResourceID:            "res1",
			LabelsHash:            123,
			PricingQuantity:       dec("4"),
			Cost:                  dec("10.4"),
			Credit:                dec("10"),
			CudCredit:             dec("9"),
			FreeCredit:            dec("8"),
			MonetaryGrantCredit:   dec("7"),
			VolumeIncentiveCredit: dec("6"),
			UpdatedAt:             qtool.UInt64Ts(time.Unix(1, 0)),
		},
		{
			BillingAccountID:      "baacc1",
			Date:                  qtool.DateString(time.Date(2000, 1, 3, 0, 0, 0, 0, time.Local)),
			CloudID:               "cloud1",
			FolderID:              "folder1",
			SkuID:                 "some-sku",
			ResourceID:            "res1",
			LabelsHash:            123,
			PricingQuantity:       dec("5"),
			Cost:                  dec("12.4"),
			Credit:                dec("12"),
			CudCredit:             dec("11"),
			FreeCredit:            dec("10"),
			MonetaryGrantCredit:   dec("9"),
			VolumeIncentiveCredit: dec("8"),
			UpdatedAt:             qtool.UInt64Ts(time.Unix(1, 0)),
		},
	}
	updatedAt := qtool.UInt64Ts(time.Unix(1, 0))
	err := suite.queries.PushResourceReports(suite.ctx, updatedAt, rows...)
	suite.Require().NoError(err)
	return rows
}

func (q *testQueries) getResourceReports(ctx context.Context) (result []ResourceReportsRow, err error) {
	ctx = tooling.QueryStarted(ctx)
	defer func() {
		tooling.QueryDone(ctx, err)
	}()

	query := getResourceReportsQuery.WithParams(q.qp)
	err = q.db.SelectContext(ctx, &result, query)
	err = qtool.WrapWithQuery(err, query)
	return
}

func dec(v string) qtool.DefaultDecimal {
	return qtool.DefaultDecimal(decimal.Must(decimal.FromString(v)))
}

func decMul(d qtool.DefaultDecimal, v string) qtool.DefaultDecimal {
	return qtool.DefaultDecimal(decimal.Decimal128(d).Mul(decimal.Must(decimal.FromString(v))))
}

var getResourceReportsQuery = qtool.Query(
	"SELECT", qtool.PrefixedCols("rs",
		"billing_account_id",
		"`date`",
		"cloud_id",
		"sku_id",
		"folder_id",
		"resource_id",
		"labels_hash",
		"pricing_quantity",
		"cost",
		"credit",
		"cud_credit",
		"monetary_grant_credit",
		"volume_incentive_credit",
		"free_credit",
		"updated_at",
	),
	"FROM", qtool.TableAs("reports/realtime/resource_reports", "rs"),
)
