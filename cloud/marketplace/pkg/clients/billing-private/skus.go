package billing

type skuManager interface {
	CreateSku(params CreateSkuParams) (Sku, error)
}

type Sku struct {
	ID string
}

type CreateSkuParams struct {
	Name             string `json:"name"`
	ServiceID        string `json:"serviceId"`
	BalanceProductID string `json:"balanceProductId"`
	MetricUnit       string `json:"metricUnit"`
	UsageUnit        string `json:"usageUnit"`
	PricingUnit      string `json:"pricingUnit"`
}

func (s *Session) CreateSku(params CreateSkuParams) (Sku, error) {
	var result struct {
		Sku Sku `json:"sku"`
	}

	response, err := s.ctxAuthRequest().
		SetBody(params).
		SetResult(&result).
		Post("/billing/v1/private/skus")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return Sku{}, err
	}

	return result.Sku, nil
}
