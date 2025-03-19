package actions

// Current (as of 2022-03-11) set of rules in PROD + PREPROD:
//
// #          entity    category           path  expected...
// "billing_account","whitelist","usage_status","paid"
// "billing_account","whitelist","usage_status","paid","service"
// "cloud_permission_stages","whitelist","MARKETPLACE_MONGODB_ENTERPRISE","exists"
//

import (
	"context"
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/dummy-backend"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/mocks"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"

	ydb_internal "a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"
)

const (
	testCloudID   = "test.cloud.id"
	testAuthToken = "test.token"
)

func TestPermissionsStagesLicenseCheck(t *testing.T) {
	suite.Run(t, new(PermissionStagesLicenseCheckTestSuite))
}

func TestBillingAccountLicenseCheck(t *testing.T) {
	suite.Run(t, new(BillingAccountLicenseCheckTestSuite))
}

type BaseLicenseCheckTestSuite struct {
	suite.Suite

	env *env.Env

	billingMock     *mocks.Billing
	rmMock          *mocks.CloudService
	defaultAuthMock *mocks.AuthTokenProvider
	ydbMock         *mocks.YDB

	logger log.Logger

	productsIDs []string
}

func (suite *BaseLicenseCheckTestSuite) SetupTest() {
	suite.logger = &zap.Logger{
		L: zaptest.NewLogger(suite.T()),
	}

	suite.env = env.NewEnvBuilder().
		WithAuthBackend(dummy.NewDummyAuthBackend()).
		WithHandlersLogger(suite.logger).
		WithBackendsFabrics(
			suite.initMocks()...,
		).
		Build()

	suite.productsIDs = []string{
		"a", "b", "c",
	}
}

func (suite *BaseLicenseCheckTestSuite) initMocks() []env.BackendsOption {
	suite.billingMock = new(mocks.Billing)
	suite.rmMock = new(mocks.CloudService)
	suite.defaultAuthMock = new(mocks.AuthTokenProvider)
	suite.ydbMock = new(mocks.YDB)

	return []env.BackendsOption{
		env.BackendsWithBilling(suite.billingMock),
		env.BackendsWithResourceManager(suite.rmMock),
		env.BackendsWithYCDefaultCredentials(suite.defaultAuthMock),
		env.BackendsWithYdb(suite.ydbMock),
	}
}

func (suite *BaseLicenseCheckTestSuite) makePermissionRulesSpec(permissions ...string) string {
	var rules []string
	for i := range permissions {
		rules = append(rules, fmt.Sprintf(`{"entity":"cloud_permission_stages","path":"%s","category":"whitelist","expected":["exists"]}`, permissions[i]))
	}

	return "[" + strings.Join(rules, ", ") + "]"
}

func (suite *BaseLicenseCheckTestSuite) makeWhiteListBillingAccountRulesSpec(expected ...string) string {
	expectedStr := `"` + strings.Join(expected, `", "`) + `"`
	return "[" +
		fmt.Sprintf(`{"entity":"billing_account","category":"whitelist","expected":[%s],"path":"usage_status"}`, expectedStr) +
		"]"
}

type PermissionStagesLicenseCheckTestSuite struct {
	BaseLicenseCheckTestSuite
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestNoPermissionsStagesWithoutRulesSpecs() {
	suite.rmMock.Mock.On("GetPermissionStages", testCloudID).Return(([]string)(nil), nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(([]ydb_internal.ProductVersion)(nil), nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestSomePermissionsStagesWithoutRules() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	suite.rmMock.Mock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(([]ydb_internal.ProductVersion)(nil), nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestGetPermissionStagesError() {
	expectedErr := fmt.Errorf("failed")

	suite.rmMock.Mock.On("GetPermissionStages", testCloudID).Return([]string{}, expectedErr)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(([]ydb_internal.ProductVersion)(nil), nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().Nil(actionResult)

	suite.Require().EqualError(err, "failed")
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestGetPermissionStagesDBFailure() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	expectedDBErr := fmt.Errorf("failed")

	suite.rmMock.Mock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(([]ydb_internal.ProductVersion)(nil), expectedDBErr)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().Nil(actionResult)

	suite.Require().EqualError(err, "failed")
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestSeveralCloudPermissionExists() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("bb2")),
		},
		{
			ID:           "c",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("aa1")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&billing.ExtendedBillingAccountCamelCaseView{}, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestCloudPermissionOneExistsOneAbsent() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("bb2")),
		},
		{
			ID:           "c",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("cc4")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&billing.ExtendedBillingAccountCamelCaseView{}, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().True(xerrors.As(err, &ErrLicenseCheckExternal{}), "unexpected error typeof [%T]: %s", err, err)
	suite.Require().Nil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestCloudPermissionExists() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("bb2")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.Mock.
		On("Get", ydb_internal.GetProductVersionsParams{
			IDs: suite.productsIDs,
		}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&billing.ExtendedBillingAccountCamelCaseView{}, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestCloudPermissionNotExists() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("cc4")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&billing.ExtendedBillingAccountCamelCaseView{}, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().True(xerrors.As(err, &ErrLicenseCheckExternal{}), "unexpected error typeof [%T]: %s", err, err)
	suite.Require().Nil(actionResult)
}

func (suite *PermissionStagesLicenseCheckTestSuite) TestDefaultAuthTokenFailure() {
	flags := []string{
		"aa1", "bb2", "cc3",
	}

	expectedErr := fmt.Errorf("boom")

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makePermissionRulesSpec("cc3")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, expectedErr)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return(flags, nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&billing.ExtendedBillingAccountCamelCaseView{}, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().EqualError(err, "boom")
	suite.Require().Nil(actionResult)
}

type BillingAccountLicenseCheckTestSuite struct {
	BaseLicenseCheckTestSuite
}

func (suite *BillingAccountLicenseCheckTestSuite) TestUsageStatusOk() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "bar",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
}

func (suite *BillingAccountLicenseCheckTestSuite) TestUsageStatusNotFound() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "zoo",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().IsType(ErrLicenseCheckExternal{}, err, "unexpected error type %T, %+v", err, err)
	suite.Require().Nil(actionResult)
}

func (suite *BillingAccountLicenseCheckTestSuite) TestDefaultAuthTokenFailure() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "foo",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	expectedErr := fmt.Errorf("boom")

	suite.defaultAuthMock.On("Token").Return(testAuthToken, expectedErr)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().EqualError(err, "boom")
	suite.Require().Nil(actionResult)
}

func (suite *BillingAccountLicenseCheckTestSuite) TestRequestBillingAccountFailed() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "zoo",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	expectedErr := fmt.Errorf("ba-boom")

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, expectedErr)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().EqualError(err, "ba-boom")
	suite.Require().Nil(actionResult)
}

func (suite *BillingAccountLicenseCheckTestSuite) TestRequestPermissionsStagesFailed() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "foo",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	expectedErr := fmt.Errorf("rm-boom")

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), expectedErr)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, nil)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().EqualError(err, "rm-boom")
	suite.Require().Nil(actionResult)
}

func (suite *BillingAccountLicenseCheckTestSuite) TestRequestProductVersoinsFailed() {
	expectedBillingAccount := billing.ExtendedBillingAccountCamelCaseView{
		UsageStatus: "foo",
	}

	productsVersions := []ydb_internal.ProductVersion{
		{
			ID:           "a",
			LicenseRules: ydb.NewAnyJSON(suite.makeWhiteListBillingAccountRulesSpec("foo", "bar")),
		},
	}

	expectedErr := fmt.Errorf("db-boom")

	suite.defaultAuthMock.On("Token").Return(testAuthToken, nil)
	suite.rmMock.On("GetPermissionStages", testCloudID).Return([]string(nil), nil)
	suite.ydbMock.On("Get", ydb_internal.GetProductVersionsParams{
		IDs: suite.productsIDs,
	}).Return(productsVersions, expectedErr)
	suite.billingMock.On("ResolveBillingAccountByCloudIDFull", testCloudID).
		Return(&expectedBillingAccount, nil)

	actionResult, err := NewLicenseCheckAction(suite.env).Do(context.Background(), LicenseCheckParams{
		CloudID:     testCloudID,
		ProductsIDs: suite.productsIDs,
	})

	suite.Require().Error(err)
	suite.Require().EqualError(err, "db-boom")
	suite.Require().Nil(actionResult)
}
