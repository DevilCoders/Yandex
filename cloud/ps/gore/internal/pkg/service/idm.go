package service

type IDM struct {
	Active *bool            `json:"active,omitempty"` // VERIFY: is ptr
	Roles  map[string]int64 `json:"roles,omitempty"`  // VERIFY: get by slug
}

func (idm *IDM) IsActive() bool {
	if idm.Active == nil {
		return false
	}
	return *idm.Active
}
