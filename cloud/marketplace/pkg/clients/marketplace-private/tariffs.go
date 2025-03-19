package marketplace

import "encoding/json"

type tariffManager interface {
	ListTariffsByProductID(publisherID, productID string) ([]Tariff, error)
	GetTariff(params GetTariffParams) (*Tariff, error)
	CreateTariff(params CreateTariffParams) (*TariffCreateOperation, error)
}

type Tariff struct {
	ID          string `json:"id"`
	ProductID   string `json:"productId"`
	PublisherID string `json:"publisherId"`
	Type        string `json:"type"`
	Name        string `json:"name"`
	State       string `json:"state"`
}

type GetTariffParams struct {
	PublisherID string `json:"publisherId"`
	ProductID   string `json:"productId"`
	TariffID    string `json:"tariffId"`
}

type FreeDefinition struct {
	Free struct{} `json:"free"`
}
type CreateTariffParams struct {
	ProductID   string      `json:"productId"`
	PublisherID string      `json:"publisherId"`
	Name        string      `json:"name"`
	Definition  interface{} `json:"definition"`
}

type TariffCreateOperation struct {
	Response    Tariff `json:"response"`
	ID          string `json:"id"`
	PublisherID string `json:"publisherId"`
	Description string `json:"description"`
	CreatedBy   string `json:"createdBy"`
	CreatedAt   string `json:"createdAt"`
	ModifiedAt  string `json:"modifiedAt"`

	Done     bool            `json:"done"`
	Metadata json.RawMessage `json:"metadata"`
	Error    OperationError  `json:"error"`
}

func (s *Session) ListTariffsByProductID(publisherID, productID string) ([]Tariff, error) {
	var result struct {
		Tariffs []Tariff `json:"tariffs"`
	}

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("publisherID", publisherID).
		SetPathParam("productID", productID).
		Get("/marketplace/v2/private/publishers/{publisherID}/products/{productID}/tariffs")
	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return result.Tariffs, nil
}

func (s *Session) GetTariff(params GetTariffParams) (*Tariff, error) {
	var result Tariff

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		SetPathParam("productID", params.ProductID).
		SetPathParam("tariffID", params.TariffID).
		Get("/marketplace/v2/private/publishers/{publisherID}/products/{productID}/tariffs/{tariffID}")
	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}

func (s *Session) CreateTariff(params CreateTariffParams) (*TariffCreateOperation, error) {
	var result TariffCreateOperation

	response, err := s.ctxAuthRequest().
		SetBody(params).
		SetResult(&result).
		SetPathParam("publisherID", params.PublisherID).
		SetPathParam("productID", params.ProductID).
		Post("/marketplace/v2/private/publishers/{publisherID}/products/{productID}/tariffs")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}
