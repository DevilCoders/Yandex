package url

import (
	"net/url"
	"testing"

	"github.com/stretchr/testify/require"
)

func mustParseURL(s string) *url.URL {
	u, err := url.Parse(s)
	if err != nil {
		panic(err)
	}

	return u
}

func TestAddrFromURL(t *testing.T) {
	inputs := []struct {
		Name string
		URL  *url.URL
		Addr string
	}{
		{
			Name: "http",
			URL:  mustParseURL("http://foo.bar"),
			Addr: "foo.bar:80",
		},
		{
			Name: "https",
			URL:  mustParseURL("https://foo.bar"),
			Addr: "foo.bar:443",
		},
		{
			Name: "http with port",
			URL:  mustParseURL("http://foo.bar:123"),
			Addr: "foo.bar:123",
		},
		{
			Name: "https with port",
			URL:  mustParseURL("https://foo.bar:123"),
			Addr: "foo.bar:123",
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			require.Equal(t, input.Addr, AddrFromURL(input.URL))
		})
	}
}
