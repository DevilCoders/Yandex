package restapi

import (
	"net/url"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestMatchStringPattern(t *testing.T) {
	t.Run("for a string", func(t *testing.T) {
		require.Equal(t, "^foo$", matchStringPattern("foo"))
	})
	t.Run("for FQDN", func(t *testing.T) {
		require.Equal(t, "^foo\\.db\\.yandex\\.net$", matchStringPattern("foo.db.yandex.net"))
	})
}

func TestBuildUrl(t *testing.T) {
	c := Client{host: "dbm"}
	t.Run("only collection ends with trailing slash", func(t *testing.T) {
		require.Equal(t,
			"https://dbm/api/v2/dom0/",
			c.buildURL("dom0", nil, nil))
	})
	t.Run("collection with resource without trailing slash", func(t *testing.T) {
		resource := "host some"
		require.Equal(t,
			"https://dbm/api/v2/dom0/host%20some",
			c.buildURL("dom0", &resource, nil))
	})
	t.Run("collection and query params", func(t *testing.T) {
		query := url.Values{}
		query.Add("foo", "bar")
		require.Equal(t,
			"https://dbm/api/v2/dom0/?foo=bar",
			c.buildURL("dom0", nil, &query))
	})
	t.Run("collection resource and query params", func(t *testing.T) {
		rID := "somehost"
		query := url.Values{}
		query.Add("foo", "bar")
		require.Equal(t,
			"https://dbm/api/v2/dom0/somehost?foo=bar",
			c.buildURL("dom0", &rID, &query))
	})
}
