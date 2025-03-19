package services

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	timestamppb "google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

var _ billing_grpc.SkuServiceServer = &SkuService{}

type SkuService struct {
	client console.Client
}

func NewSkuService(client console.Client) *SkuService {
	return &SkuService{client: client}
}

func (s SkuService) Get(ctx context.Context, request *billing_grpc.GetSkuRequest) (*billing_grpc.Sku, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.GetSkuRequest{
		AuthData: console.AuthData{Token: token},
		ID:       request.Id,
	}
	if request.Currency != "" {
		consoleReq.Currency = &request.Currency
	}
	if request.BillingAccountId != "" {
		consoleReq.BillingAccountID = &request.BillingAccountId
	}
	consoleSku, err := s.client.GetSku(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	sku := &billing_grpc.Sku{
		Id:              consoleSku.ID,
		Name:            consoleSku.Name,
		Description:     consoleSku.Description,
		ServiceId:       consoleSku.ServiceID,
		PricingUnit:     consoleSku.PricingUnit,
		PricingVersions: mapGRPCPricingVersions(consoleSku.PricingVersions),
	}

	return sku, nil
}

func (s SkuService) List(ctx context.Context, request *billing_grpc.ListSkusRequest) (*billing_grpc.ListSkusResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListSkuRequest{
		AuthData: console.AuthData{Token: token},
	}

	if request.Filter != "" {
		consoleReq.Filter = &request.Filter
	}

	if request.PageToken != "" {
		consoleReq.PageToken = &request.PageToken
	}

	if request.Currency != "" {
		consoleReq.Currency = &request.Currency
	}

	if request.PageSize != 0 {
		consoleReq.PageSize = &request.PageSize
	}

	if request.BillingAccountId != "" {
		consoleReq.BillingAccountID = &request.BillingAccountId
	}

	consoleResponse, err := s.client.ListSkus(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	var res = &billing_grpc.ListSkusResponse{
		Skus:          mapGRPCSkus(consoleResponse),
		NextPageToken: consoleResponse.NextPageToken,
	}

	return res, nil
}

func mapGRPCSkus(consoleResponse *console.ListSkuResponse) []*billing_grpc.Sku {
	var skus []*billing_grpc.Sku
	for _, consoleSku := range consoleResponse.Skus {
		skus = append(skus, &billing_grpc.Sku{
			Id:              consoleSku.ID,
			Name:            consoleSku.Name,
			Description:     consoleSku.Description,
			ServiceId:       consoleSku.ServiceID,
			PricingUnit:     consoleSku.PricingUnit,
			PricingVersions: mapGRPCPricingVersions(consoleSku.PricingVersions),
		})
	}
	return skus
}

func mapGRPCPricingExpressions(consoleResponse []console.PricingExpressionResponse) []*billing_grpc.PricingExpression {
	var expression []*billing_grpc.PricingExpression
	for _, consolePricingExpression := range consoleResponse {
		expression = append(expression, &billing_grpc.PricingExpression{
			Rates: mapGRPCRates(consolePricingExpression.Rates),
		})
	}

	return expression
}

func mapGRPCRates(consoleResponse []console.RateResponse) []*billing_grpc.Rate {
	var rates []*billing_grpc.Rate
	for _, consoleRate := range consoleResponse {
		rates = append(rates, &billing_grpc.Rate{
			StartPricingQuantity: consoleRate.StartPricingQuantity,
			UnitPrice:            consoleRate.UnitPrice,
			Currency:             consoleRate.Currency,
		})
	}

	return rates
}

func mapGRPCPricingVersions(consoleResponse []console.PricingVersionResponse) []*billing_grpc.PricingVersion {
	var versions []*billing_grpc.PricingVersion
	for _, consoleVersion := range consoleResponse {

		versionType := billing_grpc.PricingVersionType_PRICING_VERSION_TYPE_UNSPECIFIED
		if value, ok := billing_grpc.PricingVersionType_value[consoleVersion.Type]; ok {
			versionType = billing_grpc.PricingVersionType(value)
		}

		versions = append(versions, &billing_grpc.PricingVersion{
			EffectiveTime:      &timestamppb.Timestamp{Seconds: consoleVersion.EffectiveTime},
			PricingExpressions: mapGRPCPricingExpressions([]console.PricingExpressionResponse{consoleVersion.PricingExpression}),
			Type:               versionType,
		})
	}
	return versions
}
