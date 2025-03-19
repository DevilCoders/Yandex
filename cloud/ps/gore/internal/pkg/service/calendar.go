package service

type Calendar struct {
	Active  *bool `json:"active,omitempty"`
	LayerID int   `json:"layerid,omitempty"`
}

func (c *Calendar) IsActive() bool {
	return *c.Active
}
