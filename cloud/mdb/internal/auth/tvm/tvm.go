package tvm

import (
	"context"
	"net/http"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Client is
type Client interface {
	// Ping sends ping request to tvm daemon and returns non-nil error on
	// no 200 OK response
	Ping(ctx context.Context) error
	// GetServiceTicket fetches service ticket for tvmAlias from TVM
	GetServiceTicket(ctx context.Context, alias string) (string, error)
	// CheckServiceTicket checks ticket in TVM and returns ServiceTicketInfo
	CheckServiceTicket(ctx context.Context, alias, ticket string) (ServiceTicket, error)
}

// Known headers
var (
	HeaderXYaServiceTicket = http.CanonicalHeaderKey("X-Ya-Service-Ticket")
	HeaderAuthorization    = http.CanonicalHeaderKey("Authorization")
)

const (
	// OAuthTokenPrefix is prefix in HTTP header for OAuth token
	OAuthTokenPrefix = "OAuth "
)

// FormatOAuthToken for HTTP header
func FormatOAuthToken(token string) string {
	return OAuthTokenPrefix + token
}

// ParseOAuthToken from HTTP header
func ParseOAuthToken(oauth string) (string, error) {
	if !strings.HasPrefix(oauth, OAuthTokenPrefix) {
		return "", xerrors.Errorf("OAuth prefix not found: %s", oauth)
	}

	token := oauth[len(OAuthTokenPrefix):]
	token = strings.TrimSpace(token)

	if token == "" {
		return "", xerrors.Errorf("empty OAuth token: %s", oauth)
	}

	return token, nil
}

// ServiceTicket ...
type ServiceTicket struct {
	SrcID       uint32   `json:"src"`
	DstID       uint32   `json:"dst"`
	ScopesSlice []string `json:"scopes"`
	DebugStr    string   `json:"debug_string"`
	LoggingStr  string   `json:"logging_string"`
	Error       *string  `json:"error"`
}
