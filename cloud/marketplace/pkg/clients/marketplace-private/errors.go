package marketplace

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/go-resty/resty/v2"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type APIError struct {
	Code    string `json:"code"`
	Message string `json:"message"`
	Status  int    `json:"status"`
}

func (e *APIError) Error() string {
	return fmt.Sprintf("%d, %s: %s", e.Status, e.Code, e.Message)
}

var (
	// ErrNotFound resource not found.
	ErrNotFound = xerrors.New("requested resource not found")

	// ErrInternalClientError client internal error.
	ErrInternalClientError = xerrors.New("client internal error")

	// ErrInternalServerError internal server error.
	ErrInternalServerError = xerrors.New("internal server error")

	// ErrNotAuthorized no permission error.
	ErrNotAuthorized = xerrors.New("you do not have permission for the specified account to perform this action.")

	// ErrNotAuthenticated wrong (or unrecognized) credentials provided.
	ErrNotAuthenticated = xerrors.New("authentication credentials were not provided or given token not valid for any token type.")

	// ErrMethodNotAllowed wrong URI path or server is missing handlers.
	ErrMethodNotAllowed = xerrors.New("Method not allowed - check path")

	ErrBadRequest = xerrors.New("Bad request")
)

// mapError map REST api error into typed package errors.
func (s *Session) mapError(ctx context.Context, response *resty.Response, requestErr error) (resultErr error) {
	if requestErr != nil {
		// NOTE: Could be retry error, it makes sense to wrap it into additional.
		ctxlog.Error(ctx, s.logger, "failed to make request", log.Error(requestErr))

		if xerrors.Is(requestErr, context.DeadlineExceeded) || xerrors.Is(requestErr, context.Canceled) {
			return requestErr
		}

		return fmt.Errorf("%v %w", requestErr, ErrInternalClientError)
	}

	if response == nil || !response.IsError() {
		return nil
	}

	var apiErr APIError
	if err := json.Unmarshal(response.Body(), &apiErr); err != nil {
		return fmt.Errorf("error parsing APIError %v %s", err, apiErr.Error())
	}

	code, resourcePath := response.StatusCode(), response.Request.RawRequest.URL.Path

	defer func() {
		scopeLogger := log.With(s.logger,
			log.Error(resultErr),
			log.Int("http_status", code),
			log.String("resource_path", resourcePath),
		)

		if resultErr == nil {
			scopeLogger.Debug("api request completed")
			return
		}

		scopeLogger.Error("api request failed", log.Error(resultErr), log.String("api error: ", apiErr.Error()))
	}()

	switch code {
	case http.StatusNotFound:
		return ErrNotFound
	case http.StatusForbidden:
		return ErrNotAuthorized
	case http.StatusUnauthorized:
		return ErrNotAuthenticated
	case http.StatusMethodNotAllowed:
		return ErrMethodNotAllowed
	case http.StatusBadRequest:
		return ErrBadRequest
	}

	if code >= 500 {
		return ErrInternalServerError
	}

	return
}
