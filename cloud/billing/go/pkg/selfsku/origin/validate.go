package origin

import (
	"errors"
	"fmt"
	"reflect"
	"strconv"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/jmesparse"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/library/go/valid"
)

var vctx *valid.ValidationCtx

func init() {
	vctx = valid.NewValidationCtx()
	// Reflection magic
	vctx.Add("each", eachApply)
	vctx.Add("keys", eachKey)

	// Validation tags
	vctx.Add("not-empty", wrapValidator(notEmpty))
	vctx.Add("lowercase", wrapValidator(lowerCase))
	vctx.Add("positive", wrapValidator(positive))
	vctx.Add("not-negative", wrapValidator(notNegative))
	vctx.Add("name", wrapValidator(allowedName))
	vctx.Add("cloudid", wrapValidator(cloudID))
	vctx.Add("hierarchy", wrapValidator(allowedHierarchicalName))
	vctx.Add("jmespath", wrapValidator(validJMESPath))
	vctx.Add("uniq", wrapValidator(stringsUniq))
	vctx.Add("valid", wrapValidator(selfValid))
}

func notEmpty(value interface{}) error {
	switch v := value.(type) {
	case string:
		if v == "" {
			return errors.New("empty string")
		}
	case decimal.Decimal128:
		switch {
		case !v.IsFinite():
			return errors.New("should be finite number")
		case v.IsZero():
			return errors.New("should not be zero")
		}
	case time.Time:
		if v.IsZero() {
			return errors.New("empty time")
		}
	case YamlTime:
		if (time.Time)(v).IsZero() {
			return errors.New("empty time")
		}
	default:
		switch rv := reflect.ValueOf(value); rv.Kind() {
		case reflect.Slice:
			if rv.Len() == 0 {
				return errors.New("empty slice")
			}
		case reflect.Map:
			if rv.Len() == 0 {
				return errors.New("empty map")
			}
		default:
			return fmt.Errorf("unknown type for empty check: %T", value)
		}
	}
	return nil
}

func lowerCase(v string) error {
	if strings.ToLower(v) != v {
		return errors.New("should not contain upper case letters")
	}
	return nil
}

func positive(v decimal.Decimal128) error {
	if v.Sign() <= 0 {
		return errors.New("should be greater than 0")
	}
	return nil
}

func notNegative(v decimal.Decimal128) error {
	if v.Sign() < 0 {
		return errors.New("should not be negative")
	}
	return nil
}

const allowedNameNonLetterRunes = "._-"

func allowedName(v string) error {
	if v == "" {
		return errors.New("empty name")
	}
	for _, r := range v {
		switch {
		case r >= 'a' && r <= 'z':
			continue
		case r >= '0' && r <= '9':
			continue
		case strings.ContainsRune(allowedNameNonLetterRunes, r):
			continue
		}
		return fmt.Errorf("invalid name character %s", strconv.QuoteRune(r))
	}
	return nil
}

func cloudID(v string) error {
	if len(v) != 17 { // generated id's part length
		return fmt.Errorf("invalid id length %d instead of 17", len(v))
	}
	for _, r := range v {
		switch { // base32 encoding characters
		case r >= 'a' && r <= 'v':
			continue
		case r >= '0' && r <= '9':
			continue
		}
		return fmt.Errorf("invalid id character %s", strconv.QuoteRune(r))
	}
	return nil
}

func allowedHierarchicalName(v string, par string) error {
	parts := strings.Split(v, "/")
	if v == "" {
		parts = nil
	}
	if par != "" {
		partsNum, err := strconv.Atoi(par)
		if err != nil {
			return fmt.Errorf("invalid numeric parameter: %w", err)
		}
		if len(parts) != partsNum {
			return fmt.Errorf("expected %d hierarchy parts but found %d", partsNum, len(parts))
		}
	}

	for i, p := range parts {
		if err := notEmpty(p); err != nil {
			return fmt.Errorf("part %d %w", i, err)
		}
		if err := allowedName(p); err != nil {
			return err
		}
	}
	return nil
}

func validJMESPath(v string) error {
	if v == "" {
		return nil
	}
	parser := jmesparse.NewParser()
	_, err := parser.Parse(v)
	if err != nil {
		return fmt.Errorf("invalid JMESPath: %w", err)
	}
	return nil
}

func stringsUniq(v []string) error {
	_, err := tools.CheckUniqStrings(v...)
	return err
}

type selfValidator interface {
	Valid() error
}

func selfValid(v selfValidator) error {
	return v.Valid()
}
