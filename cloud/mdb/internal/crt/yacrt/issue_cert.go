package yacrt

import (
	"bytes"
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type issueRequest struct {
	Type       string `json:"type"`
	CaName     string `json:"ca_name"`
	Hosts      string `json:"hosts"`
	AbcService string `json:"abc_service,omitempty"`
}

// IssueCert does http calls to issue and download certificate
func (c *Client) IssueCert(ctx context.Context, target string, altNames []string, caName string, certType crt.CertificateType) (*crt.Cert, error) {
	hosts := make([]string, 0, len(altNames)+1)
	hosts = append(hosts, target)
	hosts = append(hosts, altNames...)
	ci, err := c.issueNew(ctx, hosts, caName, certType)
	if err != nil {
		return nil, xerrors.Errorf("certificate issue error: %w", err)
	}

	cert, err := c.downloadCert(ctx, ci.Download2)
	if err != nil {
		return nil, xerrors.Errorf("certificate download error: %w", err)
	}

	return crt.ParseCert(cert)
}

func (c *Client) issueNew(ctx context.Context, targets []string, caName string, certType crt.CertificateType) (*certInfo, error) {
	issueReq := issueRequest{
		Type:       string(certType),
		CaName:     caName,
		Hosts:      strings.Join(targets, ","),
		AbcService: c.abcService,
	}
	body, err := json.Marshal(&issueReq)
	if err != nil {
		return nil, err
	}

	request, err := http.NewRequest(http.MethodPost, c.url, bytes.NewBuffer(body))
	if err != nil {
		return nil, err
	}
	request = request.WithContext(ctx)
	request.Header.Add(headers.ContentTypeKey, headers.TypeApplicationJSON.String())
	c.addAuthHeaderVal(request.Header)

	resp, err := c.cli.Do(request, "issue_cert")
	if err != nil {
		return nil, err
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != issueSuccessCode {
		return nil, xerrors.Errorf("certificator req %v has wrong response code: %d", issueReq, resp.StatusCode)
	}
	respBody, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	ci := certInfo{}
	err = json.Unmarshal(respBody, &ci)
	if err != nil {
		return nil, err
	}

	return &ci, nil
}

func (c *Client) downloadCert(ctx context.Context, downloadLink string) ([]byte, error) {
	if downloadLink == "" {
		return nil, xerrors.Errorf("No download link")
	}

	req, err := http.NewRequest(http.MethodGet, downloadLink, nil)
	if err != nil {
		return nil, err
	}
	req = req.WithContext(ctx)
	c.addAuthHeaderVal(req.Header)
	res, err := c.cli.Do(req, "download_cert")
	if err != nil {
		return nil, err
	}
	defer func() { _ = res.Body.Close() }()
	if res.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("download cert error code: %d", res.StatusCode)
	}

	body, err := ioutil.ReadAll(res.Body)
	return body, err
}
