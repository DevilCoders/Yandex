/*
Package iam provides interface for retrieving and caching iam tokens.
*/
package yc

import (
	"context"
	"crypto/rsa"
	"crypto/x509"
	"encoding/json"
	"encoding/pem"
	"errors"
	"fmt"
	"io/ioutil"
	"sync"
	"time"

	"github.com/golang-jwt/jwt/v4"
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/credentials"
	"github.com/ydb-platform/ydb-go-sdk/v3/testutil/timeutil"
)

// Default iamClient parameters.
const (
	DefaultAudience = "https://iam.api.cloud.yandex.net/iam/v1/tokens"
	DefaultEndpoint = "iam.api.cloud.yandex.net:443"
	DefaultTokenTTL = time.Hour
)

type CreateTokenFunc func(context.Context, string) (string, time.Time, error)

var (
	ErrServiceFileInvalid = errors.New("service account file is not valid")
	ErrKeyCannotBeParsed  = errors.New("private key can not be parsed")
	ErrEndpointRequired   = errors.New("iam: endpoint required")
)

// createTokenError contains reason of token creation failure.
type createTokenError struct {
	Cause  error
	Reason string
}

// Error implements error interface.
func (e *createTokenError) Error() string {
	return fmt.Sprintf("iam: create token error: %s", e.Reason)
}

func (e *createTokenError) Unwrap() error {
	return e.Cause
}

type IamOption func(*iamClient) error

// WithIamSourceInfo set sourceInfo
func WithIamSourceInfo(sourceInfo string) IamOption {
	return func(c *iamClient) error {
		c.sourceInfo = sourceInfo
		return nil
	}
}

// WithIamKeyID set provided keyID.
func WithIamKeyID(keyID string) IamOption {
	return func(c *iamClient) error {
		c.KeyID = keyID
		return nil
	}
}

// WithIamIssuer set provided issuer.
func WithIamIssuer(issuer string) IamOption {
	return func(c *iamClient) error {
		c.Issuer = issuer
		return nil
	}
}

// WithIamTokenTTL set provided tokenTTL duration.
func WithIamTokenTTL(tokenTTL time.Duration) IamOption {
	return func(c *iamClient) error {
		c.TokenTTL = tokenTTL
		return nil
	}
}

// WithIamAudience set provided audience.
func WithIamAudience(audience string) IamOption {
	return func(c *iamClient) error {
		c.Audience = audience
		return nil
	}
}

// WithIamPrivateKey set provided private key.
func WithIamPrivateKey(key *rsa.PrivateKey) IamOption {
	return func(c *iamClient) error {
		c.Key = key
		return nil
	}
}

// WithIamPrivateKeyFile try set key from provided private key file path
func WithIamPrivateKeyFile(path string) IamOption {
	return func(c *iamClient) error {
		data, err := ioutil.ReadFile(path)
		if err != nil {
			return err
		}
		key, err := parsePrivateKey(data)
		if err != nil {
			return err
		}
		c.Key = key
		return nil
	}
}

// WithIamServiceFile try set key, keyID, issuer from provided service account file path.
// Do not mix this option with WithKeyID, WithIssuer and key options (WithPrivateKey, WithPrivateKeyFile, etc).
func WithIamServiceFile(path string) IamOption {
	return func(c *iamClient) error {
		data, err := ioutil.ReadFile(path)
		if err != nil {
			return err
		}
		type keyFile struct {
			ID               string `json:"id"`
			ServiceAccountID string `json:"service_account_id"`
			PrivateKey       string `json:"private_key"`
		}
		var info keyFile
		if err = json.Unmarshal(data, &info); err != nil {
			return err
		}
		if info.ID == "" || info.ServiceAccountID == "" || info.PrivateKey == "" {
			return ErrServiceFileInvalid
		}

		key, err := parsePrivateKey([]byte(info.PrivateKey))
		if err != nil {
			return err
		}
		c.Key = key
		c.KeyID = info.ID
		c.Issuer = info.ServiceAccountID
		return nil
	}
}

// NewIAM creates IAM (jwt) authorized iamClient from provided ClientOptions list.
// To create successfully at least one of endpoint options must be provided.
func NewIAM(createToken CreateTokenFunc, opts ...IamOption) (credentials.Credentials, error) {
	c := &iamClient{
		Audience: DefaultAudience,
		TokenTTL: DefaultTokenTTL,
	}
	for _, opt := range opts {
		err := opt(c)
		if err != nil {
			return nil, err
		}
	}
	c.createToken = createToken

	return c, nil
}

// Client contains options for interaction with the iam.
type iamClient struct {
	Key    *rsa.PrivateKey
	KeyID  string
	Issuer string

	TokenTTL time.Duration
	Audience string

	once    sync.Once
	mu      sync.RWMutex
	err     error
	token   string
	expires time.Time

	createToken CreateTokenFunc

	sourceInfo string
}

func (c *iamClient) String() string {
	if c.sourceInfo == "" {
		return "iam.Client"
	}
	return "iam.Client created from " + c.sourceInfo
}

// Token returns cached token if no c.TokenTTL time has passed or no token
// expiration deadline from the last request exceeded. In other way, it makes
// request for a new one token.
func (c *iamClient) Token(ctx context.Context) (token string, err error) {
	c.mu.RLock()
	if !c.expired() {
		token = c.token
	}
	c.mu.RUnlock()
	if token != "" {
		return token, nil
	}
	now := timeutil.Now()
	c.mu.Lock()
	defer c.mu.Unlock()
	if !c.expired() {
		return c.token, nil
	}
	var expires time.Time
	token, expires, err = c.createToken(ctx, c.jwt(now))
	if err != nil {
		return "", &createTokenError{
			Cause:  err,
			Reason: err.Error(),
		}
	}
	c.token = token
	c.expires = now.Add(c.TokenTTL)
	if expires.Before(c.expires) {
		c.expires = expires
	}
	return token, nil
}

func (c *iamClient) expired() bool {
	return c.expires.Sub(timeutil.Now()) <= 0
}

// By default Go RSA PSS uses PSSSaltLengthAuto, but RFC states that salt size
// must be equal to hash size.
//
// See https://tools.ietf.org/html/rfc7518#section-3.5
var ps256WithSaltLengthEqualsHash = &jwt.SigningMethodRSAPSS{
	SigningMethodRSA: jwt.SigningMethodPS256.SigningMethodRSA,
	Options: &rsa.PSSOptions{
		SaltLength: rsa.PSSSaltLengthEqualsHash,
	},
}

func (c *iamClient) jwt(now time.Time) string {
	var (
		issued = now.UTC().Unix()
		expire = now.Add(c.TokenTTL).UTC().Unix()
		method = ps256WithSaltLengthEqualsHash
	)
	t := jwt.Token{
		Header: map[string]interface{}{
			"typ": "JWT",
			"alg": method.Alg(),
			"kid": c.KeyID,
		},
		Claims: jwt.StandardClaims{
			Issuer:    c.Issuer,
			IssuedAt:  issued,
			Audience:  c.Audience,
			ExpiresAt: expire,
		},
		Method: method,
	}
	s, err := t.SignedString(c.Key)
	if err != nil {
		panic(fmt.Sprintf("iam: could not sign jwt token: %v", err))
	}
	return s
}

func parsePrivateKey(raw []byte) (*rsa.PrivateKey, error) {
	block, _ := pem.Decode(raw)
	if block == nil {
		return nil, ErrKeyCannotBeParsed
	}
	key, err := x509.ParsePKCS1PrivateKey(block.Bytes)
	if err == nil {
		return key, err
	}

	x, err := x509.ParsePKCS8PrivateKey(block.Bytes)
	if err != nil {
		return nil, err
	}
	if key, ok := x.(*rsa.PrivateKey); ok {
		return key, nil
	}
	return nil, ErrKeyCannotBeParsed
}

func WithIAM(createToken CreateTokenFunc, opts ...IamOption) ydb.Option {
	return ydb.WithCreateCredentialsFunc(func(ctx context.Context) (credentials.Credentials, error) {
		c, err := NewIAM(createToken, opts...)
		if err != nil {
			return nil, err
		}
		return c, nil
	})
}
