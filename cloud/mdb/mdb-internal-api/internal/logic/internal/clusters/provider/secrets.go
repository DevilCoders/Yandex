package provider

//go:generate ../../../../../../scripts/mockgen.sh ClusterSecrets

type ClusterSecrets interface {
	Generate() ([]byte, []byte, error)
}
