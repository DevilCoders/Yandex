package service

//Deprecated, но стоит оставить, если планируем, что пользоваться будут не только в облаке
type Golem struct {
	Active *bool    `json:"active,omitempty"`
	Hosts  []string `json:"hosts,omitempty"` //Список хостов
}

func (g *Golem) IsActive() bool {
	if g.Active == nil {
		return false
	}
	return *g.Active
}
