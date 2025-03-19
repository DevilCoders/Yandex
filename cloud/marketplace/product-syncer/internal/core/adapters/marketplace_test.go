package adapters

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/adapters/mocks"
)

const (
	testProductID   = "product-id"
	testPublisherID = "publisher-id"
)

func TestGetProductByID(t *testing.T) {
	suite.Run(t, new(GetProductByIDTestSuite))
}

type BaseMarketplaceAdapterTestSuite struct {
	suite.Suite

	adapter MarketplaceAdapter

	// NOTE: would be nice to have mocks for token acquisition.
	token string

	marketplaceMock *mocks.Marketplace
}

func (suite *BaseMarketplaceAdapterTestSuite) SetupTest() {
	suite.marketplaceMock = &mocks.Marketplace{}
	suite.token = "secure-token"
	suite.adapter = NewMarketplaceAdapter(suite.marketplaceMock, auth.NewTokenAuthenticator(suite.token))
}

type GetProductByIDTestSuite struct {
	BaseMarketplaceAdapterTestSuite
	productID   string
	publisherID string
}

func (suite *GetProductByIDTestSuite) SetupTest() {
	suite.BaseMarketplaceAdapterTestSuite.SetupTest()

	suite.productID = testProductID
	suite.publisherID = testPublisherID

}

func (suite *GetProductByIDTestSuite) TestSuccess() {
	productID := "test.product.id"
	publisherID := "test.publisher.id"

	expectedProduct := marketplace.Product{
		ID:             "id1",
		Name:           "yc/debian-11",
		State:          "",
		TermsOfService: nil,
		VersionID:      "",
		TariffID:       "",
		MarketingInfo:  marketplace.MarketingInfo{},
		Source:         marketplace.Source{},
		Payload:        marketplace.Payload{},
		PublisherID:    "random-publisher",
		Type:           "compute",
	}

	params := marketplace.GetProductParams{
		ProductID:   productID,
		PublisherID: publisherID,
	}

	suite.marketplaceMock.On("GetProductByID", params).Return(&expectedProduct, nil)
	result, err := suite.adapter.GetProductByID(context.Background(), publisherID, productID)

	suite.Require().NoError(err)
	suite.Require().NotNil(result)

	suite.Require().Equal(expectedProduct.ID, result.ID)
	suite.Require().Equal(expectedProduct.Name, result.Name)
	suite.Require().Equal(expectedProduct.Type, result.Type)
}

func (suite *GetProductByIDTestSuite) TestEmptyResponseFailure() {
	productID := "test.product.id"
	publisherID := "test.publisher.id"

	expected := &marketplace.Product{}

	params := marketplace.GetProductParams{
		ProductID:   productID,
		PublisherID: publisherID,
	}

	suite.marketplaceMock.On("GetProductByID", params).Return(expected, ErrNotFound)
	result, err := suite.adapter.GetProductByID(context.Background(), publisherID, productID)

	suite.Require().Error(err)
	suite.Require().Nil(result)
}
