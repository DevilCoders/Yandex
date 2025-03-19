package blackbox

import "context"

// Client to blackbox
type Client interface {
	Ping(ctx context.Context) error
	OAuth(ctx context.Context, tvmToken, token, userip string, dbfields []string) (UserInfo, error)
	SessionID(ctx context.Context, tvmToken, sessionID, userip, host string, dbfields []string) (UserInfo, error)
}

// DisplayName ...
type DisplayName struct {
	Name string `json:"name"`
}

// UserInfo ...
type UserInfo struct {
	DBFields    map[string]string
	DisplayName DisplayName
	Login       string
	UID         string
	Scope       string
}
