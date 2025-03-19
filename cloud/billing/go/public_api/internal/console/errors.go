package console

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/scope"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrRequest            = errors.New("can't perform request")
	ErrUnexpectedResponse = errors.New("unexpected response")

	ErrMarshalResponse = xerrors.NewSentinel("can't marshal response")

	ErrUnauthenticated   = xerrors.NewSentinel("unauthenticated")
	ErrPermissionDenied  = xerrors.NewSentinel("permission denied")
	ErrNotFound          = xerrors.NewSentinel("not found")
	ErrResourceExhausted = xerrors.NewSentinel("resource exhausted")

	ErrInvalidRequest  = xerrors.NewSentinel("invalid request")
	ErrServerInternal  = xerrors.NewSentinel("internal server error")
	ErrResponseGeneral = xerrors.NewSentinel("console client http error")
)

type ResponseErr struct {
	StatusCode int
	Message    string
}

func (err *ResponseErr) Error() string {
	return fmt.Sprintf(" (code=%v, message=%v)", err.StatusCode, err.Message)
}

func parseResponseErr(resp *resty.Response) *ResponseErr {
	responseErr := &ResponseErr{
		StatusCode: resp.StatusCode(),
	}

	var data struct {
		Message string `json:"message"`
	}

	if err := json.Unmarshal(resp.Body(), &data); err == nil {
		responseErr.Message = data.Message
	}

	return responseErr
}

func wrapResponseErr(responseErr *ResponseErr) error {
	switch code := responseErr.StatusCode; {
	case code == 400:
		return ErrInvalidRequest.Wrap(responseErr)
	case code == 401:
		return ErrUnauthenticated.Wrap(responseErr)
	case code == 403:
		return ErrPermissionDenied.Wrap(responseErr)
	case code == 404:
		return ErrNotFound.Wrap(responseErr)
	case code == 429:
		return ErrResourceExhausted.Wrap(responseErr)
	case code >= 500:
		return ErrServerInternal.Wrap(responseErr)
	}
	return ErrResponseGeneral.Wrap(responseErr)
}

func handleErrorResponse(ctx context.Context, response *resty.Response) error {
	responseErr := parseResponseErr(response)
	scope.Logger(ctx).Error("console http response error",
		logging.HTTPStatusCode(responseErr.StatusCode),
		logging.ConsoleMessage(responseErr.Message),
	)

	responseErr.Message = "" // TODO (goncharov-art): check all possible error messages
	return wrapResponseErr(responseErr)
}
