package blackbox

import (
	"errors"
	"fmt"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrNoDefaultUID                = errors.New("blackbox: empty_default_uid")
	ErrNoDefaultUser               = errors.New("blackbox: default_user_not_in_session")
	ErrRequestNoOAuthToken         = errors.New("blackbox: request_no_oauth_token")
	ErrRequestNoSessionID          = errors.New("blackbox: request_no_session_id")
	ErrRequestNoHost               = errors.New("blackbox: request_no_host")
	ErrRequestNoUserIP             = errors.New("blackbox: request_no_userip")
	ErrRequestTvmNotAvailable      = errors.New("blackbox: request_tvm_not_available")
	ErrRequestNoEmailToTest        = errors.New("blackbox: request_no_email_to_test")
	ErrRequestNoUIDOrLogin         = errors.New("blackbox: request_no_uid_or_login")
	ErrRequestNoUserTicket         = errors.New("blackbox: request_no_user_ticket")
	ErrRequestUIDsandMultiConflict = errors.New("blackbox: request_uids_and_multi_conflict")
	ErrRequestNoGetPhones          = errors.New("blackbox: request_no_getphones")
)

var _ xerrors.Wrapper = (*UnauthorizedError)(nil)

// UnauthorizedError means user is not authorized.
// Probably you want to redirect user to auth page (passport or oauth).
type UnauthorizedError struct {
	StatusError
}

func (e *UnauthorizedError) Unwrap() error {
	return e.StatusError
}

// IsUnauthorized checks if error mean unauthorized user.
func IsUnauthorized(err error) bool {
	_, unauthorized := err.(*UnauthorizedError)
	return unauthorized
}

// StatusError means unexpected credential (oauth token or session) status.
type StatusError struct {
	Status  Status
	Message string
}

func (e StatusError) Error() string {
	return fmt.Sprintf("blackbox: %s: %s", e.Status, e.Message)
}
