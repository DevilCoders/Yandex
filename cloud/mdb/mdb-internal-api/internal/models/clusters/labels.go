package clusters

import "a.yandex-team.ru/cloud/mdb/internal/valid"

type Labels map[string]string

var (
	labelKeyValidator = valid.MustStringComposedValidator(
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "label key %q is too short",
			TooLongMsg:  "label key %q is too long",
		},
		&valid.Regexp{
			Pattern: "^[a-z][-_./\\\\@0-9a-z]*$",
			Msg:     "label key %q has invalid symbols",
		},
	)

	labelValueValidator = valid.MustStringComposedValidator(
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "label value %q is too short",
			TooLongMsg:  "label value %q is too long",
		},
		&valid.Regexp{
			Pattern: "^[-_./\\\\@0-9a-z]*$",
			Msg:     "label value %q has invalid symbols",
		},
	)
)

func (l Labels) Validate() error {
	for k, v := range l {
		if err := labelKeyValidator.ValidateString(k); err != nil {
			return err
		}
		if err := labelValueValidator.ValidateString(v); err != nil {
			return err
		}
	}

	return nil
}
