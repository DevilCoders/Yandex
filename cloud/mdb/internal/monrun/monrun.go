package monrun

import (
	"fmt"
	"regexp"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ResultCode int

const (
	OK   ResultCode = 0
	WARN ResultCode = 1
	CRIT ResultCode = 2
)

const (
	okCodeStr   = "OK"
	warnCodeStr = "WARN"
	critCodeStr = "CRIT"
)

func (rc ResultCode) String() string {
	switch rc {
	case OK:
		return okCodeStr
	case WARN:
		return warnCodeStr
	case CRIT:
		return critCodeStr
	}
	panic(fmt.Sprintf("Unexpected ResultCode: %d", rc))
}

func ResultCodeFromString(s string) (ResultCode, error) {
	switch s {
	case okCodeStr:
		return OK, nil
	case warnCodeStr:
		return WARN, nil
	case critCodeStr:
		return CRIT, nil
	}
	return OK, xerrors.Errorf("Unknown ResultCode: %s", s)
}

// Result is monrun result
type Result struct {
	Code    ResultCode
	Message string
}

var _sanitizeReg *regexp.Regexp

func init() {
	_sanitizeReg = regexp.MustCompile("[;\n]+")
}

func sanitizeMessage(message string) string {
	return _sanitizeReg.ReplaceAllLiteralString(message, " ")
}

func (c Result) String() string {
	message := sanitizeMessage(c.Message)
	if c.Code == OK && len(message) == 0 {
		message = "OK"
	}
	return fmt.Sprintf("%d;%s", c.Code, message)
}

// Warnf return WARN Result
func Warnf(format string, a ...interface{}) Result {
	return Result{
		Code:    WARN,
		Message: fmt.Sprintf(format, a...),
	}
}

// Critf return CRIT Result
func Critf(format string, a ...interface{}) Result {
	return Result{
		Code:    CRIT,
		Message: fmt.Sprintf(format, a...),
	}
}
