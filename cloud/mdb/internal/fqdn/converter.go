package fqdn

//go:generate ../../scripts/mockgen.sh Converter

type Converter interface {
	ManagedToUnmanaged(fqdn string) (string, error)
	UnmanagedToManaged(fqdn string) (string, error)
}
