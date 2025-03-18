package urlutil

import (
	"errors"
	"net"
	"net/url"
	"strings"
)

var (
	ErrNoHosts = errors.New("no hosts found")
)

type MultihostURL struct {
	*url.URL
	Hosts []string
}

// NewMultihostURLFromString parses valid URL string with multiple hosts delimited by comma and returns a MultihostURL object.
// It behalves as url.URL with various helpers.
// Example:
//     mhu, _ := urlutil.NewMultihostURLFromString("postgres://username:pass@host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6432/mydb?sslmode=verify-full")
//	   for _, host := range mhu.Hosts {
//	       fmt.Println(host)
//	   }
//     // host1.sas.yc.yandex.net:5432
//     // host2.man.yc.yandex.net:6432
func NewMultihostURLFromString(u string) (*MultihostURL, error) {
	uu, err := url.Parse(u)
	if err != nil {
		return nil, err
	}

	return NewMultihostURL(uu)
}

// NewMultihostURL parses *url.URL with multiple hosts delimited by comma and returns a MultihostURL object.
// It behalves as url.URL with various helpers.
// Example:
//     u, _ := url.Parse("postgres://username:pass@host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6432/mydb?sslmode=verify-full")
//     mhu, _ := urlutil.NewMultihostURL(u)
//	   for _, host := range mhu.Hosts {
//	       fmt.Println(host)
//	   }
//     // host1.sas.yc.yandex.net:5432
//     // host2.man.yc.yandex.net:6432
func NewMultihostURL(u *url.URL) (*MultihostURL, error) {
	hostname := u.Host
	if hostname == "" {
		return nil, ErrNoHosts
	}
	hosts := strings.Split(hostname, ",")
	defaultPort := u.Port()

	mh := make([]string, len(hosts))
	for i, host := range hosts {
		host = strings.TrimSpace(host)

		_, p, _ := net.SplitHostPort(host)
		if p == "" && defaultPort != "" {
			host += ":" + defaultPort
		}

		mh[i] = host
	}

	uu := *u
	if u.User != nil {
		up := *u.User
		uu.User = &up
	}

	mhu := MultihostURL{
		URL:   &uu,
		Hosts: mh,
	}

	return &mhu, nil
}

// URLs constructs a slice of valid *url.URL for each hosts in MultihostURL
// Example:
//     mhu, _ := urlutil.ParseMultihostURLString("postgres://username:pass@host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6432/mydb?sslmode=verify-full")
//     us := mhu.URLs()
//     // postgres://username:pass@host1.sas.yc.yandex.net:5432/mydb?sslmode=verify-full
//     // postgres://username:pass@host2.man.yc.yandex.net:6432/mydb?sslmode=verify-full
func (u *MultihostURL) URLs() []*url.URL {
	res := make([]*url.URL, len(u.Hosts))
	for i, host := range u.Hosts {
		uu := *u.URL
		if u.User != nil {
			up := *u.User
			uu.User = &up
		}
		uu.Host = host

		res[i] = &uu
	}

	return res
}
