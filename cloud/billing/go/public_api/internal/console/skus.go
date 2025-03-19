package console

import (
	"context"
	"encoding/json"
	"strconv"
)

type GetSkuRequest struct {
	AuthData
	ID               string
	Currency         *string
	BillingAccountID *string
}

type ListSkuRequest struct {
	AuthData
	Currency         *string
	Filter           *string
	PageToken        *string
	PageSize         *int64
	BillingAccountID *string
}

type RateResponse struct {
	StartPricingQuantity string `json:"startPricingQuantity"`
	UnitPrice            string `json:"unitPrice"`
	Currency             string `json:"currency"`
}

type PricingExpressionResponse struct {
	Rates []RateResponse `json:"rates"`
}

type PricingVersionResponse struct {
	EffectiveTime     int64                     `json:"effectiveTime"`
	PricingExpression PricingExpressionResponse `json:"pricingExpression"`
	Type              string                    `json:"type"`
}

type Sku struct {
	ID              string                   `json:"id"`
	Name            string                   `json:"name"`
	Description     string                   `json:"description"`
	ServiceID       string                   `json:"serviceId"`
	PricingUnit     string                   `json:"pricingUnit"`
	PricingVersions []PricingVersionResponse `json:"pricingVersions"`
}

type ListSkuResponse struct {
	Skus          []Sku  `json:"skus"`
	NextPageToken string `json:"nextPageToken"`
}

func (c *consoleClient) GetSku(ctx context.Context, req *GetSkuRequest) (*Sku, error) {

	url := "/skus/{id}"
	request := c.createRequest(ctx, &req.AuthData)
	request.SetPathParam("id", req.ID)

	if req.Currency != nil {
		request = request.SetQueryParam("currency", *req.Currency)
	}
	if req.BillingAccountID != nil {
		request = request.SetQueryParam("billingAccountId", *req.BillingAccountID)
	}

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &Sku{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
func (c *consoleClient) ListSkus(ctx context.Context, req *ListSkuRequest) (*ListSkuResponse, error) {

	url := "/skus"
	request := c.createRequest(ctx, &req.AuthData)
	if req.Currency != nil {
		request = request.SetQueryParam("currency", *req.Currency)
	}
	if req.PageSize != nil {
		request = request.SetQueryParam("pageSize", strconv.FormatInt(*req.PageSize, 10))
	}
	if req.PageToken != nil {
		request = request.SetQueryParam("pageToken", *req.PageToken)
	}
	if req.BillingAccountID != nil {
		request = request.SetQueryParam("billingAccountId", *req.BillingAccountID)
	}
	if req.Filter != nil {
		request = request.SetQueryParam("filter", *req.Filter)
	}

	response, err := request.Get(url)
	if err != nil {
		return nil, ErrRequest
	}

	if response.StatusCode() != 200 {
		return nil, handleErrorResponse(ctx, response)
	}

	res := &ListSkuResponse{}
	err = json.Unmarshal(response.Body(), res)
	if err != nil {
		return nil, ErrUnexpectedResponse
	}

	return res, nil
}
