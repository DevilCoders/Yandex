package httplaas

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/laas"
)

const (
	DefaultHTTPHost = "http://laas.yandex.ru"
)

var _ laas.Client = new(Client)

type Client struct {
	httpc *resty.Client
}

// NewClientWithResty returns new LaaS HTTP client based on custom resty client and options.
func NewClientWithResty(httpc *resty.Client, opts ...ClientOpt) (*Client, error) {
	c := &Client{
		httpc: httpc,
	}
	for _, o := range opts {
		if err := o(c); err != nil {
			return nil, err
		}
	}

	if c.httpc.HostURL == "" {
		c.httpc.SetBaseURL(DefaultHTTPHost)
	}
	c.httpc.SetHeader("Host", c.httpc.HostURL)

	c.httpc.SetDoNotParseResponse(true)

	return c, nil
}

// NewHTTPClient returns new LaaS HTTP client using given options.
func NewClient(opts ...ClientOpt) (*Client, error) {
	return NewClientWithResty(resty.New(), opts...)
}

// DetectRegion sends request to LaaS user region detection handler using given request params.
func (c Client) DetectRegion(ctx context.Context, p laas.Params) (*laas.RegionResponse, error) {
	req := c.httpc.R()

	// add params to request
	if len(p.UserIP) > 0 && !p.UserIP.IsUnspecified() {
		req.SetHeader("X-Real-IP", p.UserIP.String())
	}
	if p.URLPrefix != "" {
		req.SetHeader("X-Url-Prefix", p.URLPrefix)
	}

	cookies := make([]string, 0, 3)
	if p.YandexUID > 0 {
		cookies = append(cookies, "yandexuid="+strconv.FormatUint(p.YandexUID, 10))
	}
	if p.YandexGID > 0 {
		cookies = append(cookies, "yandex_gid="+strconv.FormatInt(int64(p.YandexGID), 10))
	}
	if p.YP != "" {
		cookies = append(cookies, "yp="+p.YP)
	}
	if len(cookies) > 0 {
		req.SetHeader("Cookie", strings.Join(cookies, "; "))
	}

	resp, err := req.SetContext(ctx).Get("/region")
	if err != nil {
		return nil, xerrors.Errorf("laas: %w", err)
	}

	// read all and close body for proper Keep-Alive connection reuse
	defer func() {
		_, _ = io.Copy(ioutil.Discard, resp.RawBody())
		_ = resp.RawBody().Close()
	}()

	if resp.StatusCode() != http.StatusOK {
		return nil, fmt.Errorf("laas: bad HTTP status code %d", resp.StatusCode())
	}

	var result laas.RegionResponse

	dec := json.NewDecoder(resp.RawBody())
	if err := dec.Decode(&result); err != nil {
		return nil, xerrors.Errorf("laas: %w", err)
	}

	return &result, nil
}
