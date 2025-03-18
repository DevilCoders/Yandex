package model

import (
	"errors"
	"fmt"
	"reflect"
	"regexp"
	"strings"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

var (
	fqdnRegexp             = regexp.MustCompile(`^([a-zA-Z0-9]{1}[a-zA-Z0-9_-]{0,62})(\.[a-zA-Z0-9_]{1}[a-zA-Z0-9_-]{0,62})*?(\.[a-zA-Z]{1}[a-zA-Z0-9]{0,62})\.?$`)
	originsGroupNameRegexp = regexp.MustCompile("^[a-z0-9_-]+")

	customHostRegexp         = regexp.MustCompile(`^[\w.-]+`)
	allowedOriginRegexp      = regexp.MustCompile(`^https?://[\w.-]+$`)
	whitelistBlacklistRegexp = regexp.MustCompile("^[a-z_]")
	compressTypesRegexp      = regexp.MustCompile("^[a-z-]/[a-z-+.]")
	headerRegexp             = regexp.MustCompile("^[a-zA-Z-]+")
	headerNameRegexp         = regexp.MustCompile("^[a-zA-Z-]+")

	ruleNameRegexp = regexp.MustCompile(`^[a-z][a-z0-9_]*`)

	bannedReplacementGroup = regexp.MustCompile(`\$(?:\{(\D)|([^\d{]))`)
	replacementGroupRegexp = regexp.MustCompile(`\$(?:\{(\d+)}|(\d+))`)
)

func isRegexp(value *inspection.Inspected) error {
	if k := value.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %s", k, "invalid type")
	}

	_, err := regexp.Compile(value.Indirect.String())
	if err != nil {
		return err
	}

	return nil
}

func secondaryHostnameRule(value *inspection.Inspected) error {
	if k := value.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %s", k, "invalid type")
	}

	stringValue := value.Indirect.String()

	var host string
	parts := strings.SplitN(stringValue, "/", 2)
	if len(parts) < 1 {
		return errors.New("invalid value")
	}
	host = parts[0]

	if !fqdnRegexp.MatchString(host) {
		return errors.New("value not contain fqdn")
	}

	return nil
}

var headerNameForbiddenValuesMap = map[string]struct{}{
	"content-length":    {},
	"transfer-encoding": {},
}

func isNotHeaderNameForbiddenValue(value *inspection.Inspected) error {
	if k := value.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %s", k, "invalid type")
	}

	str := value.Indirect.String()
	if _, ok := headerNameForbiddenValuesMap[strings.ToLower(str)]; ok {
		return fmt.Errorf("forbidden value: %s", str)
	}

	return nil
}

func validateHeaders(headers []string) error {
	containNames := false
	containSpecialValues := false
	handledValues := make(map[string]struct{})
	allowedValues := map[string]struct{}{
		"*":             {},
		"authorization": {},
	}

	for _, header := range headers {
		lower := strings.ToLower(header)
		if _, ok := handledValues[lower]; !ok {
			handledValues[lower] = struct{}{}
		} else {
			return fmt.Errorf("found duplicate value: %s", header)
		}

		if lower == "*" || lower == "authorization" {
			containSpecialValues = true
			if containNames {
				return errors.New("invalid headers value")
			}
			if _, ok := allowedValues[lower]; ok {
				delete(allowedValues, lower)
			} else {
				return errors.New("invalid headers value")
			}
			continue
		}

		if containSpecialValues {
			return errors.New("invalid headers value")
		}

		containNames = true
		if !headerRegexp.MatchString(lower) {
			return errors.New("invalid headers value")
		}
	}

	return nil
}

func formatError(err error) error {
	return fmt.Errorf("%+v", err)
}
