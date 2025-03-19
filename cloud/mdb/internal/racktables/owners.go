package racktables

type Owner struct {
	Type string `json:"type"`
	Name string `json:"name"`
}

const OwnerTypeService = "service"
const OwnerTypeServiceRole = "servicerole"
const OwnerTypeUser = "user"

type GetMacrosWithOwnersResponse map[string]struct {
	Owners []Owner `json:"owners"`
}

type MacrosWithOwners map[string][]Owner
