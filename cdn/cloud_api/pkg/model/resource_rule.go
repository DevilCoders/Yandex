package model

type ResourceRule struct {
	ID              int64
	ResourceID      string
	ResourceVersion int64

	Name                 string
	Pattern              string
	OriginsGroupEntityID *int64
	OriginProtocol       *OriginProtocol
	Options              *ResourceOptions
}

type CreateResourceRuleParams struct {
	ResourceID      string
	ResourceVersion *int64
	Name            string
	Pattern         string
	OriginsGroupID  *int64
	OriginProtocol  *OriginProtocol
	Options         *ResourceOptions
}

type GetResourceRuleParams struct {
	RuleID     int64
	ResourceID string
}

type GetAllResourceRulesParams struct {
	ResourcePairs []ResourceIDVersionPair
}

type ResourceIDVersionPair struct {
	ID      string
	Version int64
}

type GetAllRulesByResourceParams struct {
	ResourceID      string
	ResourceVersion *int64
}

type UpdateResourceRuleParams struct {
	RuleID          int64
	ResourceID      string
	ResourceVersion *int64
	Name            string
	Pattern         string
	OriginsGroupID  *int64
	OriginProtocol  *OriginProtocol
	Options         *ResourceOptions
}

type DeleteResourceRuleParams struct {
	RuleID     int64
	ResourceID string
}
