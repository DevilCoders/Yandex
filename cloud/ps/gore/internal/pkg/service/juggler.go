package service

type Juggler struct {
	Active      *bool             `json:"active,omitempty"`      // VERIFY: is ptr
	Namespace   string            `json:"namespace,omitempty"`   // VERIFY: get by slug
	TLPosition  int               `json:"tlposition,omitempty"`  // VERIFY: no //Место, считая от 0, куда воткнуть тимлида
	IncludeRest *bool             `json:"includeRest,omitempty"` // VERIFY: is ptr
	Rules       map[string]string `json:"rules,omitempty"`       // VERIFY: get by id //Пары имя_правила - rule_id
}

func (j *Juggler) IsActive() bool {
	return *j.Active
}
