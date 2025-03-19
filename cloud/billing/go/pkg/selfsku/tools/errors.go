package tools

import (
	"fmt"

	"a.yandex-team.ru/library/go/valid"
)

type ValidErrors struct {
	valid.Errors
}

func (ve *ValidErrors) Collect(errs ...error) {
	for _, err := range errs {
		if err == nil {
			continue
		}
		if errs, ok := err.(valid.Errors); ok {
			ve.Collect(errs...)
			continue
		}
		ve.Errors = append(ve.Errors, err)
	}
}

func (ve *ValidErrors) CollectWrapped(format string, err error) {
	if err == nil {
		return
	}
	if errs, ok := err.(valid.Errors); ok {
		for _, err := range errs {
			ve.CollectWrapped(format, err)
		}
		return
	}
	ve.Collect(fmt.Errorf(format, err))
}

func (ve ValidErrors) Expose() error {
	if len(ve.Errors) > 0 {
		return ve.Errors
	}
	return nil
}
