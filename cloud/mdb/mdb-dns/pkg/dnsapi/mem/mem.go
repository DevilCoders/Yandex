package mem

import (
	"context"
	"fmt"
	"sync"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dnsapi.Client = &Client{}

// Client in mem for dnsapi
type Client struct {
	records    cidfqdnMap
	upCount    int
	failCount  int
	mux        sync.Mutex
	failTarget map[string]string
}

// Errors
var (
	errFailedUpdate = xerrors.NewSentinel("failed update")
)

// New constructs secretsstore mock
func New() *Client {
	return &Client{
		records: make(cidfqdnMap),
	}
}

// UpdateRecords process update info for DNS API
func (c *Client) UpdateRecords(ctx context.Context, up *dnsapi.RequestUpdate) ([]*dnsapi.RequestUpdate, error) {
	c.mux.Lock()
	defer c.mux.Unlock()

	for _, info := range up.Records {
		_, exist := c.failTarget[info.CNAMENew]
		if exist {
			c.failCount++
			return dnsapi.SplitOnParts(up), errFailedUpdate
		}
	}
	c.upCount++
	for cidfqdn, info := range up.Records {
		fmt.Printf(" *** mem up, cidfqdn: %s, target: %s\n", cidfqdn, info.CNAMENew)
		ci := c.prepcidInfo(cidfqdn)
		ci.fqdn = info.CNAMENew
		ci.upCount++
	}
	return nil, nil
}

// GetUpdateRecordsCallCount return balk update call counter
func (c *Client) GetUpdateRecordsCallCount() int {
	return c.upCount
}

// GetUpdateRecordsFailedCount return balk update call counter
func (c *Client) GetUpdateRecordsFailedCount() int {
	return c.failCount
}

// GetRecord return data for cidfqdn and update count or 0 if was no update
func (c *Client) GetRecord(cidfqdn string) (string, int) {
	ci, ok := c.records[cidfqdn]
	if !ok {
		return "", 0
	}
	return ci.fqdn, ci.upCount
}

// SetFailedTarget set failed cids
func (c *Client) SetFailedTarget(fc map[string]string) {
	c.failTarget = fc
}

func (c *Client) prepcidInfo(cidfqdn string) *cidInfo {
	ci, ok := c.records[cidfqdn]
	if !ok {
		ci = &cidInfo{}
		c.records[cidfqdn] = ci
	}
	return ci
}

type cidInfo struct {
	fqdn    string
	upCount int
}

type cidfqdnMap map[string]*cidInfo
