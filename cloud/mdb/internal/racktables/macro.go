package racktables

type Macro struct {
	IDs              []MacroID `json:"ids"`
	Name             string    `json:"name"`
	Owners           []string  `json:"owners,omitempty"`
	OwnerService     string    `json:"owner_service,omitempty"`
	Parent           string    `json:"parent,omitempty"`
	Description      string    `json:"description,omitempty"`
	Internet         int       `json:"internet"`
	Secured          int       `json:"secured"`
	CanCreateNetwork int       `json:"can_create_network"`
}

type MacroID struct {
	ID          string `json:"id"`
	Description string `json:"description,omitempty"`
}
