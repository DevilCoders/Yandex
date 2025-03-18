package model

import (
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

func (p *CreateResourceRuleParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
		valid.Value(&p.Name, rule.Required, rule.MustMatch(ruleNameRegexp)),
		valid.Value(&p.Pattern, rule.Required, isRegexp),
		valid.Value(&p.Options),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *GetResourceRuleParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.RuleID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *GetAllRulesByResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *UpdateResourceRuleParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.RuleID, rule.Required),
		valid.Value(&p.ResourceID, rule.Required),
		valid.Value(&p.Name, rule.Required),
		valid.Value(&p.Pattern, rule.Required),
		valid.Value(&p.Options),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *DeleteResourceRuleParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.RuleID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}
