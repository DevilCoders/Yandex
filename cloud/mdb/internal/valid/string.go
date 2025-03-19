package valid

import (
	"path"
	"regexp"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type InputErrorFormatter struct {
	OmitMessageFormatting bool
}

func (ef InputErrorFormatter) FormatError(s string, a ...interface{}) error {
	if !ef.OmitMessageFormatting {
		return semerr.InvalidInputf(s, a...)
	}
	return semerr.InvalidInput(s)
}

// Regexp validates string against regular expression
type Regexp struct {
	Pattern string
	Msg     string
	r       *regexp.Regexp
	InputErrorFormatter
}

var _ StringValidator = &Regexp{}

func (v *Regexp) Validate(_ *valid.ValidationCtx) (bool, error) {
	r, err := regexp.Compile(v.Pattern)
	if err != nil {
		return false, err
	}

	v.r = r
	return false, nil
}

func (v *Regexp) ValidateString(s string) error {
	if !v.r.MatchString(s) {
		return v.FormatError(v.Msg, s)
	}

	return nil
}

// StringLength checks that string is within minimum and maximum length
type StringLength struct {
	Min         int
	Max         int
	TooShortMsg string
	TooLongMsg  string
	InputErrorFormatter
}

var _ StringValidator = &StringLength{}

func (v *StringLength) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, valid.StringLenConstraints(v.Min, v.Max)
}

func (v *StringLength) ValidateString(s string) error {
	if err := valid.StringLen(s, v.Min, v.Max); err != nil {
		if xerrors.Is(err, valid.ErrStringTooShort) {
			return v.FormatError(v.TooShortMsg, s)
		}

		if xerrors.Is(err, valid.ErrStringTooLong) {
			return v.FormatError(v.TooLongMsg, s)
		}

		return err
	}

	return nil
}

// StringBlacklist validates that string is not blacklisted
type StringBlacklist struct {
	Blacklist []string
	Msg       string
	InputErrorFormatter
}

var _ StringValidator = &StringBlacklist{}

func (v *StringBlacklist) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v *StringBlacklist) ValidateString(s string) error {
	for _, invalid := range v.Blacklist {
		if s == invalid {
			return v.FormatError(v.Msg, s)
		}
	}

	return nil
}

type PrefixBlacklist struct {
	Blacklist []string
	Msg       string
	InputErrorFormatter
}

var _ StringValidator = &PrefixBlacklist{}

func (v *PrefixBlacklist) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v *PrefixBlacklist) ValidateString(s string) error {
	for _, invalidPrefix := range v.Blacklist {
		if strings.HasPrefix(s, invalidPrefix) {
			return v.FormatError(v.Msg, s)
		}
	}

	return nil
}

// StringValidator defines common interface for string validation
type StringValidator interface {
	valid.Validator

	ValidateString(name string) error
}

// StringComposedValidator provides single StringValidator interface over multiple validators
type StringComposedValidator struct {
	validators []StringValidator
}

var _ StringValidator = &StringComposedValidator{}

// NewStringComposedValidator composes multiple string validators
func NewStringComposedValidator(validators ...StringValidator) (*StringComposedValidator, error) {
	vctx := valid.NewValidationCtx()
	v := &StringComposedValidator{validators: validators}
	if _, err := v.Validate(vctx); err != nil {
		return nil, err
	}

	return v, nil
}

// MustStringComposedValidator composes multiple string validators. Panics on error
func MustStringComposedValidator(validators ...StringValidator) *StringComposedValidator {
	v, err := NewStringComposedValidator(validators...)
	if err != nil {
		panic(err)
	}
	return v
}

func (v *StringComposedValidator) Validate(vctx *valid.ValidationCtx) (bool, error) {
	for _, v := range v.validators {
		if _, err := v.Validate(vctx); err != nil {
			return false, err
		}
	}

	return false, nil
}

func (v *StringComposedValidator) ValidateString(s string) error {
	for _, v := range v.validators {
		if err := v.ValidateString(s); err != nil {
			return err
		}
	}

	return nil
}

// PathCanonical validates that string is in canonical form from os.path view
type PathCanonical struct {
	Msg string
	InputErrorFormatter
}

var _ StringValidator = &PathCanonical{}

func (v *PathCanonical) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v *PathCanonical) ValidateString(s string) error {
	if s != path.Clean(s) {
		return v.FormatError(v.Msg, s)
	}
	return nil
}

// PathAbsolute validates that string is absolute file path
type PathAbsolute struct {
	Msg string
	InputErrorFormatter
}

var _ StringValidator = &PathAbsolute{}

func (v *PathAbsolute) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v *PathAbsolute) ValidateString(s string) error {
	if !path.IsAbs(s) {
		return v.FormatError(v.Msg, s)
	}
	return nil
}

// PathPartsValidator validates that every part of path conforms to some validaor
type PathPartsValidator struct {
	PartValidator StringValidator
	Msg           string
	InputErrorFormatter
}

var _ StringValidator = &PathPartsValidator{}

func (v *PathPartsValidator) Validate(ctx *valid.ValidationCtx) (bool, error) {
	return v.PartValidator.Validate(ctx)
}

func (v *PathPartsValidator) ValidateString(s string) error {
	p := s
	for p != "" && p != "/" {
		d, f := path.Split(p)
		p = strings.TrimSuffix(d, "/")
		if err := v.PartValidator.ValidateString(f); err != nil {
			return v.FormatError(v.Msg, s)
		}
	}
	return nil
}
