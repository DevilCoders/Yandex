package mdbporto

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
)

func NewTestFormatter() *Formatter {
	return &Formatter{
		fqdnConverter: fqdn.NewConverter("", UserFQDNSuffix, ControlFQDNSuffix),
	}
}

func TestGetFormattedRecordsFromIPsSingleControlFQDN(t *testing.T) {
	ipProperty := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f"
	hostname := "test.db.yandex.net"

	formattedRecords, err := NewTestFormatter().GetFormattedRecordsFromIPs(ipProperty, hostname)
	require.NoError(t, err)

	expected := "0.1 test.db.yandex.net 2a02:6b8:0:51e:1234:4321:8c34:6e8f\n"

	assert.Equal(t, expected, formattedRecords)
}

func TestGetFormattedRecordsFromIPsSingleUserFQDN(t *testing.T) {
	ipProperty := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f"
	hostname := "test.mdb.yandex.net"

	formattedRecords, err := NewTestFormatter().GetFormattedRecordsFromIPs(ipProperty, hostname)
	require.NoError(t, err)

	expected := "0.1 test.mdb.yandex.net 2a02:6b8:0:51e:1234:4321:8c34:6e8f\n"

	assert.Equal(t, expected, formattedRecords)
}

func TestGetFormattedRecordsFromIPsDualControlFQDN(t *testing.T) {
	ipProperty := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f;eth0 2a02:6b8:0:51e:0:1589:8c34:6e8f"
	hostname := "test.db.yandex.net"

	formattedRecords, err := NewTestFormatter().GetFormattedRecordsFromIPs(ipProperty, hostname)
	require.NoError(t, err)

	expected := "0.1 test.db.yandex.net 2a02:6b8:0:51e:1234:4321:8c34:6e8f\n0.1 test.mdb.yandex.net 2a02:6b8:0:51e:0:1589:8c34:6e8f\n"

	assert.Equal(t, expected, formattedRecords)
}

func TestGetFormattedRecordsFromIPsDualUserFQDN(t *testing.T) {
	ipProperty := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f;eth0 2a02:6b8:0:51e:0:1589:8c34:6e8f"
	hostname := "test.mdb.yandex.net"

	formattedRecords, err := NewTestFormatter().GetFormattedRecordsFromIPs(ipProperty, hostname)
	require.NoError(t, err)

	expected := "0.1 test.db.yandex.net 2a02:6b8:0:51e:1234:4321:8c34:6e8f\n0.1 test.mdb.yandex.net 2a02:6b8:0:51e:0:1589:8c34:6e8f\n"

	assert.Equal(t, expected, formattedRecords)
}

func TestRFC5952IPv6(t *testing.T) {
	ipProperty := "eth0 2a02:6b8:c13:e82:0:0:4b5f:0ab5"
	hostname := "test.db.yandex.net"

	formattedRecords, err := NewTestFormatter().GetFormattedRecordsFromIPs(ipProperty, hostname)
	require.NoError(t, err)

	expected := "0.1 test.db.yandex.net 2a02:6b8:c13:e82::4b5f:ab5\n"

	assert.Equal(t, expected, formattedRecords)
}
