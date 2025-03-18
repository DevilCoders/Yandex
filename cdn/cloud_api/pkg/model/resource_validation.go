package model

import (
	"errors"
	"fmt"
	"math"
	"regexp/syntax"
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

func (p *CreateResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.FolderID, rule.Required),
		valid.Value(&p.OriginVariant),
		valid.Value(&p.Options),
		valid.Value(&p.SecondaryHostnames, rule.Each(secondaryHostnameRule)),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (v *OriginVariant) Validate() error {
	switch {
	case v.GroupID > 0,
		v.Source != nil:
		return nil
	}

	return errors.New("invalid origin variant")
}

func (o *ResourceOptions) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.CustomHost, rule.OmitEmpty(rule.MustMatch(customHostRegexp))),
		valid.Value(&o.CustomSNI, rule.OmitEmpty(rule.MustMatch(customHostRegexp))),
		valid.Value(&o.CORS),
		valid.Value(&o.BrowserCacheOptions),
		valid.Value(&o.EdgeCacheOptions),
		valid.Value(&o.NormalizeRequestOptions),
		valid.Value(&o.CompressionOptions),
		valid.Value(&o.StaticHeadersOptions),
		valid.Value(&o.RewriteOptions),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *CORSOptions) Validate() error {
	var validateRules []valid.ValueRule

	if o.Enabled != nil && *o.Enabled {
		validateRules = append(validateRules, valid.Value(&o.Mode, rule.Required))
	}

	if o.Mode != nil && *o.Mode == CORSModeOriginFromList {
		validateRules = append(validateRules, valid.Value(&o.AllowedOrigins, rule.Each(rule.MustMatch(allowedOriginRegexp))))
	}

	err := validateHeaders(o.AllowedHeaders)
	if err != nil {
		return fmt.Errorf("allowed headers: %w", err)
	}

	err = validateHeaders(o.ExposeHeaders)
	if err != nil {
		return fmt.Errorf("expose headers: %w", err)
	}

	validateRules = append(validateRules, valid.Value(&o.MaxAge, rule.OmitEmpty(rule.InRange(0, 86400))))

	err = valid.Struct(o,
		validateRules...,
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *BrowserCacheOptions) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.MaxAge, rule.OmitEmpty(rule.InRange(0, math.MaxInt32))),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *EdgeCacheOptions) Validate() error {
	for _, v := range o.OverrideTTLCodes {
		if v.Code < 100 || v.Code > 600 {
			return fmt.Errorf("http code must be in the range 100-599 inclusive, not %d", v.Code)
		}
	}

	err := valid.Struct(o,
		valid.Value(&o.TTL, rule.OmitEmpty(rule.InRange(0, math.MaxInt32))),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *NormalizeRequestOptions) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.QueryString),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (v *NormalizeRequestQueryString) Validate() error {
	err := valid.Struct(v,
		valid.Value(&v.Whitelist, rule.OmitEmpty(rule.MustMatch(whitelistBlacklistRegexp))),
		valid.Value(&v.Blacklist, rule.OmitEmpty(rule.MustMatch(whitelistBlacklistRegexp))),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *CompressionOptions) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.Variant),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *CompressionVariant) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.Compress),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *Compress) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.Types, rule.OmitEmpty(rule.Each(rule.MustMatch(compressTypesRegexp)))),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (o *StaticHeadersOptions) Validate() error {
	for _, val := range o.Request {
		err := val.Validate()
		if err != nil {
			return formatError(err)
		}
	}

	for _, val := range o.Response {
		err := val.Validate()
		if err != nil {
			return formatError(err)
		}
	}

	return nil
}

func (o *HeaderOption) Validate() error {
	err := valid.Struct(o,
		valid.Value(&o.Name, rule.MustMatch(headerNameRegexp), isNotHeaderNameForbiddenValue),
	)
	if err != nil {
		return formatError(err)
	}

	for _, ch := range o.Value {
		switch {
		case ch == 9:
		case ch >= 32 && ch <= 126:
		case ch == '\n' || ch == '\t':
		default:
			return fmt.Errorf("found forbidden character: %d", ch)
		}
	}

	return nil
}

func (o *RewriteOptions) Validate() error {
	if o.Regex != nil {
		regex, err := syntax.Parse(*o.Regex, syntax.Perl)
		if err != nil {
			return fmt.Errorf("failed to parse regex: %w", err)
		}
		for _, capName := range regex.CapNames() {
			if capName != "" {
				return fmt.Errorf("found named capture group: %s", capName)
			}
		}

		maxCap := regex.MaxCap()
		if o.Replacement != nil {
			bannedSequences := bannedReplacementGroup.FindAllString(*o.Replacement, -1)
			if len(bannedSequences) > 0 {
				return fmt.Errorf("found banned strings variables in replacement")
			}

			vars := replacementGroupRegexp.FindAllString(*o.Replacement, -1)
			if len(vars) > maxCap {
				return fmt.Errorf("in regexp found %d capture groups, but in replacemnt %d", maxCap, len(vars))
			}

			minPossibleVariable := 1
			maxPossibleVariable := maxCap
			for _, varStr := range vars {
				varStr = strings.TrimPrefix(varStr, "$")
				varStr = strings.TrimPrefix(varStr, "{")
				varStr = strings.TrimSuffix(varStr, "}")
				variable, err := strconv.Atoi(varStr)
				if err != nil {
					return fmt.Errorf("can't convert variable from replacement to int")
				}

				if variable < minPossibleVariable || variable > maxPossibleVariable {
					return fmt.Errorf("invalid variable number from replacement: %d, must be between %d and %d",
						variable, minPossibleVariable, maxPossibleVariable)
				}
			}
		}
	}

	return nil
}

func (p *GetResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *UpdateResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
		valid.Value(&p.Options),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *DeleteResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *ActivateResourceParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
		valid.Value(&p.Version, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *ListResourceVersionsParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.ResourceID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}

func (p *CountActiveResourcesParams) Validate() error {
	err := valid.Struct(p,
		valid.Value(&p.FolderID, rule.Required),
	)
	if err != nil {
		return formatError(err)
	}

	return nil
}
