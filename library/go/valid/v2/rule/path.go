package rule

import (
	"fmt"
	"path/filepath"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// IsAbs checks if given string value is an absolute filesystem path
func IsAbs(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	if !filepath.IsAbs(v.Indirect.String()) {
		return ErrPatternMismatch
	}
	return nil
}

// IsAbsDir checks if given string value is an absolute filesystem path to directory
func IsAbsDir(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if !filepath.IsAbs(s) || !strings.HasSuffix(s, string(filepath.Separator)) {
		return ErrPatternMismatch
	}
	return nil
}
