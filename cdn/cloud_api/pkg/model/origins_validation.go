package model

import (
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

func (p *CreateOriginParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.FolderID, rule.Required),
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.OriginParams),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *OriginParams) Validate() error {
	var validateRules []valid.ValueRule

	switch p.Type {
	case OriginTypeCommon:
		validateRules = append(validateRules, valid.Value(&p.Source, rule.MustMatch(fqdnRegexp)))
	case OriginTypeBucket, OriginTypeWebsite:
		// TODO: rules
		validateRules = append(validateRules, valid.Value(&p.Source, rule.Len(3, 63)))
	}

	err := valid.Struct(p,
		validateRules...,
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *GetOriginParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.OriginID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *GetAllOriginParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}
func (p *UpdateOriginParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.FolderID, rule.Required),
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.OriginID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}
func (p *DeleteOriginParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.OriginID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *CreateOriginsGroupParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.FolderID, rule.Required),
		valid.Value(&p.Name, rule.MustMatch(originsGroupNameRegexp)),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *GetOriginsGroupParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *UpdateOriginsGroupParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.Name, rule.MustMatch(originsGroupNameRegexp)),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *DeleteOriginsGroupParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *ActivateOriginsGroupParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
		valid.Value(&p.Version, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *ListOriginsGroupVersionsParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.OriginsGroupID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}
