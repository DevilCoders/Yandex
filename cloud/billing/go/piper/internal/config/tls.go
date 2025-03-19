package config

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"os"
)

type tlsContainer struct {
	tlsOnce   initSync
	tlsConfig *tls.Config
}

func (c *Container) GetTLS() (*tls.Config, error) {
	err := c.tlsOnce.Do(c.loadTLS)
	return c.tlsConfig, err
}

func (c *Container) loadTLS() error {
	if c.initFailed() {
		return c.initError
	}

	cfg, err := c.GetTLSConfig()
	if err != nil {
		return err
	}

	rootCAs, err := c.loadCA(cfg.CAPath)
	if err != nil {
		return c.failInitF("TLS load: %w", err)
	}
	c.tlsConfig = &tls.Config{MinVersion: tls.VersionTLS12, RootCAs: rootCAs}
	return nil
}

func (c *Container) loadCA(path string) (*x509.CertPool, error) {
	if path == "" {
		return nil, nil
	}
	p, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("CA file: %w", err)
	}

	roots, err := x509.SystemCertPool()
	if err != nil {
		return nil, fmt.Errorf("system cert pool: %w", err)
	}

	if ok := roots.AppendCertsFromPEM(p); !ok {
		return nil, fmt.Errorf("failed to append certificates")
	}

	return roots, nil
}
