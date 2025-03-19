package auth

import "context"

type YDBTokenAuthenticator struct {
	TokenGetter
}

func (a *YDBTokenAuthenticator) Token(ctx context.Context) (string, error) {
	if err := ctx.Err(); err != nil {
		return "", err
	}
	return a.GetToken()
}
