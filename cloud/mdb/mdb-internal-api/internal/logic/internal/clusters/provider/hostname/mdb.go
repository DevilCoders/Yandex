package hostname

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MDBHostnameGenerator struct {
	generator.PlatformHostnameGenerator
}

func NewMdbHostnameGenerator(g generator.PlatformHostnameGenerator) ClusterHostnameGenerator {
	return MDBHostnameGenerator{g}
}

var _ ClusterHostnameGenerator = &MDBHostnameGenerator{}

func (g MDBHostnameGenerator) GenerateFQDN(_ environment.CloudType, _ clusters.Type, geoName string, _ string, _ int, _ string,
	domain string, platform compute.Platform) (string, error) {
	prefix := fmt.Sprintf("%s-", geoName)
	suffix := fmt.Sprintf(".%s", domain)
	id, err := g.PlatformHostnameGenerator.Generate(prefix, suffix, platform)
	if err != nil {
		return "", xerrors.Errorf("fqdn not generated: %w", err)
	}

	return id, nil
}
