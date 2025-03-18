package bbtypes

import "fmt"

type BlackboxErrorCode uint8

// Documentation for BB err codes: https://docs.yandex-team.ru/blackbox/concepts/blackboxErrors
const (
	BlackboxErrorCodeOK            BlackboxErrorCode = 0
	BlackboxErrorCodeInvalidParams BlackboxErrorCode = 2
	BlackboxErrorCodeDBException   BlackboxErrorCode = 10
	BlackboxErrorCodeAccessDenied  BlackboxErrorCode = 21
	BlackboxErrorCodeUnknown       BlackboxErrorCode = 1
)

func (c BlackboxErrorCode) String() string {
	switch c {
	case BlackboxErrorCodeOK:
		return "OK"
	case BlackboxErrorCodeInvalidParams:
		return "INVALID_PARAMS"
	case BlackboxErrorCodeDBException:
		return "DB_EXCEPTION"
	case BlackboxErrorCodeAccessDenied:
		return "ACCESS_DENIED"
	case BlackboxErrorCodeUnknown:
		return "UNKNOWN"
	default:
		return fmt.Sprintf("UNKNOWN_%d", uint32(c))
	}
}

// BlackboxError is a server side blackbox error.
type BlackboxError struct {
	Code    BlackboxErrorCode
	Message string
}

func (e BlackboxError) Error() string {
	return fmt.Sprintf("blackbox: %s: %s", e.Code, e.Message)
}

func (e BlackboxError) Retryable() bool {
	/*
		https://docs.yandex-team.ru/blackbox/concepts/blackboxErrors says:
		Примечание. При возникновении ошибок DB_EXCEPTION в боевом окружении рекомендуется повторить запрос к Черному ящику, подождав несколько сотен миллисекунд.
	*/
	return e.Code == BlackboxErrorCodeDBException
}

type HTTPStatusError struct {
	Code int
	Body []byte
}

func (e HTTPStatusError) Error() string {
	return fmt.Sprintf("blackbox: unexpected http-status code: %d", e.Code)
}

type ParsingError struct {
	Err  error
	Body []byte
}

func (e ParsingError) Error() string {
	return fmt.Sprintf("blackbox: parse error: %s", e.Err.Error())
}
