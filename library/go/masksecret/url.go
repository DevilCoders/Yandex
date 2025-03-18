package masksecret

import (
	"errors"
	"net/url"
	"strings"
)

// URLString masks sensitive data in given URL string
func URLString(s string) (string, error) {
	if s == "" {
		return s, nil
	}

	u, err := url.Parse(s)
	if err != nil {
		return "", err
	}

	err = URL(u)
	if err != nil {
		return "", err
	}

	return u.String(), nil
}

// URLCopy masks sensitive data in *url.URL object.
// It will return new URL object without altering original
func URLCopy(u *url.URL) (*url.URL, error) {
	if u == nil {
		return nil, errors.New("masksecret: nil pointer given")
	}

	uu := *u
	err := URL(&uu)
	return &uu, err
}

// URL masks sensitive data in *url.URL object.
// It WILL ALTER original object
func URL(u *url.URL) (err error) {
	if u == nil {
		return errors.New("masksecret: nil pointer given")
	}

	if pass, ok := u.User.Password(); ok {
		username := u.User.Username()
		// strip username if raw password consists only of placeholder chars
		if consistsOf(pass, SecretPlaceholder) {
			username = String(username)
		} else {
			pass = String(pass)
		}
		u.User = url.UserPassword(username, pass)
	}

	if u.RawQuery != "" {
		args, err := url.ParseQuery(u.RawQuery)
		if err == nil {
			for arg := range args {
				if _, ok := markers[strings.ToLower(arg)]; ok {
					for i, v := range args[arg] {
						args[arg][i] = String(v)
					}
				}
			}
			u.RawQuery = args.Encode()
		}
	}

	return nil
}
