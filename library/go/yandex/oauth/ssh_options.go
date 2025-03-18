package oauth

import (
	"io"
	"strconv"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type (
	Option func(*options) error

	options struct {
		endpoint         string
		clientID         string
		clientSecret     string
		identity         string
		identityType     string
		randReader       io.Reader
		time             time.Time
		sshKeyring       *SSHContainerKeyring
		additionalFields [][2]string
		log              log.Structured
		timeout          time.Duration
	}
)

// WithEndpoint sets custom endpoint.
func WithEndpoint(endpoint string) Option {
	return func(opts *options) error {
		opts.endpoint = endpoint
		return nil
	}
}

// WithUserUID sets user uid.
//
// Warning: you can set user login or uid, not both.
func WithUserUID(uid int) Option {
	return func(opts *options) error {
		if opts.identity != "" {
			return xerrors.New("you can specify only login OR uid, not both")
		}

		opts.identity = strconv.Itoa(uid)
		opts.identityType = "uid"
		return nil
	}
}

// WithUserLogin sets user login.
//
// Default: current system user name.
//
// Warning: you can set username or uid, not both.
func WithUserLogin(login string) Option {
	return func(opts *options) error {
		if opts.identity != "" {
			return xerrors.New("you can specify only login OR uid, not both")
		}

		opts.identity = login
		opts.identityType = "login"
		return nil
	}
}

// WithAdditionalField sets additional field to send with token exchange request.
func WithAdditionalField(key, value string) Option {
	return func(opts *options) error {
		opts.additionalFields = append(opts.additionalFields, [2]string{key, value})
		return nil
	}
}

// WithLogger sets logger. Most of messages are logged with Debug lvl, due to library usage nature.
func WithLogger(l log.Structured) Option {
	return func(opts *options) error {
		opts.log = l
		return nil
	}
}

// WithSSHKeyring sets custom SSH keyring.
//
// Default: SSHAgentKeyring and SSHFileKeyring with common paths for private keys (e.g. ~/.ssh/id_rsa).
func WithSSHKeyring(keyrings ...SSHKeyring) Option {
	return func(opts *options) error {
		opts.sshKeyring = NewSSHContainerKeyring(keyrings...)
		return nil
	}
}

// WithTimeout sets timeout for all of exchange.
//
// Default: 5 sec
func WithTimeout(timeout time.Duration) Option {
	return func(opts *options) error {
		opts.timeout = timeout
		return nil
	}
}
