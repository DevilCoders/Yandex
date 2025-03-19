package impl

import (
	"fmt"
	"strings"

	fqdnlib "a.yandex-team.ru/cloud/mdb/internal/fqdn"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Converter struct {
	ControlplaneFQDNSuffix       string
	UnmanagedDataplaneFQDNSuffix string
	ManagedDataplaneFQDNSuffix   string
}

func (c *Converter) ManagedToUnmanaged(fqdn string) (string, error) {
	if c.ControlplaneFQDNSuffix == "" && c.UnmanagedDataplaneFQDNSuffix == "" && c.ManagedDataplaneFQDNSuffix == "" {
		return fqdn, nil
	}
	i := strings.Index(fqdn, ".")
	if i == -1 {
		return "", xerrors.Errorf("no dot: %w", fqdnlib.ErrInvalidFQDN)
	}

	if i+1 == len(fqdn) {
		return "", xerrors.Errorf("empty suffix: %w", fqdnlib.ErrInvalidFQDN)
	}

	prefix := fqdn[:i]
	suffix := fqdn[i+1:]

	// Do nothing with control plane fqdns
	if suffix == c.ControlplaneFQDNSuffix {
		return fqdn, nil
	}

	// Do nothing with unmanaged data plane fqdns
	if suffix == c.UnmanagedDataplaneFQDNSuffix {
		return fqdn, nil
	}

	// This must be managed data plane suffix
	if suffix != c.ManagedDataplaneFQDNSuffix {
		return "", xerrors.Errorf("suffix %q when supposed to be %q: %w", suffix, c.ManagedDataplaneFQDNSuffix, fqdnlib.ErrInvalidFQDN)
	}

	return fmt.Sprintf("%s.%s", prefix, c.UnmanagedDataplaneFQDNSuffix), nil
}

func (c *Converter) UnmanagedToManaged(fqdn string) (string, error) {
	if c.ControlplaneFQDNSuffix == "" && c.UnmanagedDataplaneFQDNSuffix == "" && c.ManagedDataplaneFQDNSuffix == "" {
		return fqdn, nil
	}
	i := strings.Index(fqdn, ".")
	if i == -1 {
		return "", xerrors.Errorf("no dot: %w", fqdnlib.ErrInvalidFQDN)
	}

	if i+1 == len(fqdn) {
		return "", xerrors.Errorf("empty suffix: %w", fqdnlib.ErrInvalidFQDN)
	}

	prefix := fqdn[:i]
	suffix := fqdn[i+1:]

	// Do nothing with control plane fqdns
	if suffix == c.ControlplaneFQDNSuffix {
		return fqdn, nil
	}

	// Do nothing with managed data plane fqdns
	if suffix == c.ManagedDataplaneFQDNSuffix {
		return fqdn, nil
	}

	// This must be unmanaged data plane suffix
	if suffix != c.UnmanagedDataplaneFQDNSuffix {
		return "", xerrors.Errorf("suffix %q when supposed to be %q: %w", suffix, c.UnmanagedDataplaneFQDNSuffix, fqdnlib.ErrInvalidFQDN)
	}

	return fmt.Sprintf("%s.%s", prefix, c.ManagedDataplaneFQDNSuffix), nil
}

func NewConverter(ControlplaneFQDNSuffix, UnamangedDataplaneFQDNSuffix, ManagedDataplaneFQDNSuffix string) fqdnlib.Converter {
	return &Converter{
		ControlplaneFQDNSuffix:       ControlplaneFQDNSuffix,
		UnmanagedDataplaneFQDNSuffix: UnamangedDataplaneFQDNSuffix,
		ManagedDataplaneFQDNSuffix:   ManagedDataplaneFQDNSuffix,
	}
}
