package origin

import (
	"fmt"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/library/go/valid"
)

// UnitConversion represents record about conversion one unit to another
type UnitConversion struct {
	SrcUnit string             `yaml:"src_unit" valid:"not-empty"`
	DstUnit string             `yaml:"dst_unit" valid:"not-empty"`
	Factor  decimal.Decimal128 `yaml:"factor" valid:"positive"`
}

func (u UnitConversion) Valid() (err error) {
	errs := tools.ValidErrors{}
	errs.Collect(valid.Struct(vctx, u))
	if u.SrcUnit != "" && u.SrcUnit == u.DstUnit {
		errs.Collect(fmt.Errorf("source and destination units should not have same value '%s'", u.SrcUnit))
	}
	return errs.Expose()
}

// Service stores info about YCloud service and its high level checks for billing metrics
type Service struct {
	ID          string `yaml:"id" valid:"cloudid"`
	Name        string `yaml:"name" valid:"name"`
	Description string `yaml:"description" valid:"not-empty"`
	Group       string `yaml:"group" valid:"not-empty"`
}

func (s Service) Valid() error {
	errs := tools.ValidErrors{}
	errs.Collect(valid.Struct(vctx, s))
	return errs.Expose()
}

// TagChecks stores lists of schemas tags checks
type TagChecks map[string]SchemaTagChecks

func (tc TagChecks) Valid() error {
	errs := tools.ValidErrors{}
	wrap := struct {
		TagChecks `valid:"keys=name,each=valid"`
	}{tc}

	errs.Collect(valid.Struct(vctx, wrap))
	return errs.Expose()
}

// SchemaTagChecks stores lists of required and optional tag keys in billing metrics
type SchemaTagChecks struct {
	Required []string `yaml:"required" valid:"uniq,each=not-empty"`
	Optional []string `yaml:"optional" valid:"uniq,each=not-empty"`
}

func (tc SchemaTagChecks) Valid() error {
	errs := tools.ValidErrors{}
	req := tools.NewStringSet(tc.Required...)
	opt := tools.NewStringSet(tc.Optional...)
	if optDups := req.Intersect(opt); len(optDups) != 0 {
		errs.Collect(fmt.Errorf("tag should be required or optional: %v", optDups.Items()))
	}
	return errs.Expose()
}
