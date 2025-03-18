package oauth

import (
	"bytes"
	"context"
	"crypto/rand"
	"crypto/tls"
	"encoding/base64"
	"encoding/json"
	"errors"
	"net/http"
	"net/url"
	"os"
	"os/user"
	"strconv"
	"strings"
	"time"

	"github.com/go-resty/resty/v2"
	"golang.org/x/crypto/ssh"

	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	DefaultEndpoint = "https://oauth.yandex-team.ru/token"
	DefaultTimeout  = 5 * time.Second
)

type (
	oauthResponse struct {
		Token string `json:"access_token"`
	}

	oauthErrorResponse struct {
		Error string `json:"error_description"`
	}
)

// GetTokenBySSH exchanges ssh key to oauth token for given oauth-application.
//
// Documentation: https://wiki.yandex-team.ru/oauth/token/#granttypesshkey
func GetTokenBySSH(ctx context.Context, clientID string, clientSecret string, opts ...Option) (string, error) {
	o := &options{
		endpoint:     DefaultEndpoint,
		clientID:     clientID,
		clientSecret: clientSecret,
		log:          &nop.Logger{},
		timeout:      DefaultTimeout,
		time:         time.Now(),
		randReader:   rand.Reader,
	}

	for _, opt := range opts {
		if err := opt(o); err != nil {
			return "", err
		}
	}

	// lazy detect current user
	if o.identity == "" {
		login, err := currentUser()
		if err != nil {
			return "", xerrors.Errorf("failed to get current user: %w", err)
		}

		_ = WithUserLogin(login)(o)
	}

	// lazy initialize keyring
	if o.sshKeyring == nil {
		keyring, err := AutodetectSSHKeyring()
		if err != nil {
			return "", err
		}

		o.sshKeyring = NewSSHContainerKeyring(keyring...)
		defer func() { _ = o.sshKeyring.Close() }()
	}

	return exchangeTokenBySSH(ctx, o)
}

func exchangeTokenBySSH(ctx context.Context, opts *options) (string, error) {
	ctx, cancel := context.WithTimeout(ctx, opts.timeout)
	defer cancel()

	ts := strconv.FormatInt(opts.time.Unix(), 10)
	signBuf := new(bytes.Buffer)
	signBuf.WriteString(ts)
	signBuf.WriteString(opts.clientID)
	signBuf.WriteString(opts.identity)
	signData := signBuf.Bytes()

	postData := url.Values{
		"client_id":       {opts.clientID},
		"client_secret":   {opts.clientSecret},
		"grant_type":      {"ssh_key"},
		"ts":              {ts},
		opts.identityType: {opts.identity},
	}
	for _, field := range opts.additionalFields {
		postData.Set(field[0], field[1])
	}

	var (
		httpClient = newRestyClient()
		sign       *ssh.Signature
		err        error
		token      string
	)

	for opts.sshKeyring.Next() {
		select {
		case <-ctx.Done():
			return "", ctx.Err()
		default:
		}

		signer := opts.sshKeyring.Signer()
		sign, err = signer.Sign(opts.randReader, signData)
		if err != nil {
			opts.log.Debug(
				"signing failed",
				log.String("key_fingerprint", ssh.FingerprintSHA256(signer.PublicKey())),
				log.Error(err),
			)
			continue
		}

		pubKey := signer.PublicKey()
		pubKeyType := pubKey.Type()
		switch {
		case strings.Contains(pubKeyType, "cert"):
			// OAuth supports SSH certificates
			base64Pub := base64.RawURLEncoding.EncodeToString(pubKey.Marshal())
			postData.Set("public_cert", base64Pub)
		case pubKeyType == ssh.KeyAlgoRSA:
			// or RSA key
			postData.Del("public_cert")
		default:
			// everything else is unacceptable by the server side, so we don't need to send it at all
			continue
		}

		base64Sign := base64.RawURLEncoding.EncodeToString(sign.Blob)
		postData.Set("ssh_sign", base64Sign)
		token, err = tryToGetToken(ctx, httpClient, opts.endpoint, postData)
		if err == nil {
			break
		}

		postData.Set("ssh_sign", "xxx")
		opts.log.Debug(
			"failed to get oauth-token by ssh-key",
			log.String("key_fingerprint", ssh.FingerprintSHA256(signer.PublicKey())),
			log.String("request", postData.Encode()),
			log.Error(err),
		)

		if _, fine := err.(*ServerError); !fine {
			// in case of any unexpected error (e.g. oauth server down) - stop trying
			break
		}
	}

	if err != nil {
		return "", xerrors.Errorf("last err: %w", err)
	}

	if token == "" {
		return "", errors.New("no acceptable keys available")
	}

	return token, nil
}

func tryToGetToken(ctx context.Context, httpc *resty.Client, endpoint string, postData url.Values) (string, error) {
	resp, err := httpc.R().
		SetContext(ctx).
		SetHeader("Content-Type", "application/x-www-form-urlencoded").
		SetBody(postData.Encode()).
		Post(endpoint)
	if err != nil {
		return "", xerrors.Errorf("failed to call oauth server: %w", err)
	}

	switch resp.StatusCode() {
	case http.StatusOK:
		var result oauthResponse
		if err := json.Unmarshal(resp.Body(), &result); err != nil {
			return "", xerrors.Errorf("failed to parse oauth response: %q", string(resp.Body()))
		}
		return result.Token, nil
	case http.StatusBadRequest:
		var result oauthErrorResponse
		if err := json.Unmarshal(resp.Body(), &result); err != nil {
			return "", xerrors.Errorf("failed to parse oauth response: %q", string(resp.Body()))
		}
		return "", &ServerError{message: result.Error}
	default:
		return "", xerrors.Errorf("unexpected response status: %d", resp.StatusCode())
	}
}

func newRestyClient() *resty.Client {
	r := resty.New()
	certPool, err := certifi.NewCertPool()
	if err == nil {
		r.SetTLSClientConfig(&tls.Config{RootCAs: certPool})
	}

	return r
}

func currentUser() (string, error) {
	for _, key := range []string{"SUDO_USER", "USER"} {
		if envUser := os.Getenv(key); envUser != "" {
			return envUser, nil
		}
	}

	sysUser, err := user.Current()
	if err != nil {
		return "", err
	}

	return sysUser.Username, nil
}
