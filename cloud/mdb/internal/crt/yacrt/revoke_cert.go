package yacrt

import (
	"context"
	"fmt"
	"net/http"
)

func (c *Client) RevokeByAPIURL(ctx context.Context, APIURL string) error {
	request, err := http.NewRequest(http.MethodDelete, APIURL, nil)
	if err != nil {
		return err
	}
	request = request.WithContext(ctx)
	c.addAuthHeaderVal(request.Header)

	resp, err := c.cli.Do(request, "revoke_certificate")
	if err != nil {
		return err
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("certificator unexpected response code: %d", resp.StatusCode)
	}
	return nil
}

// Revoke all certificates by hostname
func (c *Client) RevokeCertsByHostname(ctx context.Context, hostname string) (int, error) {
	revokeCnt := 0
	certList, err := c.listCertificates(ctx, hostname)
	if err != nil {
		return revokeCnt, err
	}
	for _, cert := range certList.Results {
		if cert.Status == CertStatusIssued &&
			cert.Revoked == "" {
			if err := c.RevokeByAPIURL(ctx, cert.APIURL); err != nil {
				return revokeCnt, fmt.Errorf("revoked %d of %d total certs: %w", revokeCnt, certList.Count, err)
			}
			revokeCnt++
		}
	}
	return revokeCnt, nil
}
