package chmodels

import "a.yandex-team.ru/cloud/mdb/internal/valid"

const mlModelNamePattern = "^[a-zA-Z0-9_-]+$"

type MLModel struct {
	ClusterID string
	Name      string
	Type      MLModelType
	URI       string
}

type MLModelType string

const (
	CatBoostMLModel MLModelType = "catboost"
)

// NewMLModelNameValidator constructs validator for ML model names
func NewMLModelNameValidator(pattern string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "ML model name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "ML model name %q is too short",
			TooLongMsg:  "ML model name %q is too long",
		},
	)
}

// MustMLModelNameValidator constructs validator for ML model names. Panics on error.
func MustMLModelNameValidator(pattern string) *valid.StringComposedValidator {
	v, err := NewMLModelNameValidator(pattern)
	if err != nil {
		panic(err)
	}

	return v
}

var MLModelNameValidator = MustMLModelNameValidator(mlModelNamePattern)
