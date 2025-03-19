package model

const (
	PendingTariffState string = "pending"
	ActiveTariffState  string = "active"
)

const (
	PAYGTariffType string = "payg"
)

type Tariff struct {
	ID          string `json:"id"`
	ProductID   string `json:"productId"`
	PublisherID string `json:"publisherId"`
	Type        string `json:"type"`
	Name        string `json:"name"`
	State       string `json:"state"`
}
