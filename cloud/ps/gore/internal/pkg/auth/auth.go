package auth

type AuthType int

const (
	OAuth AuthType = iota
	BasicAuth
	Nop
)

type OAuthArgs struct {
	Token string
	Login string
}

type BasicArgs struct {
	XOAuthToken string
	Login       string
}

type NopArgs struct {
}

type Auth struct {
	Method AuthType
	OAuth  *OAuthArgs
	Basic  *BasicArgs
	Nop    *NopArgs
}
