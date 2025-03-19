package grpc

import (
	"io/ioutil"
	"testing"

	"github.com/stretchr/testify/require"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/test/yatest"
)

func Test_quotaNameConstantsPresentInQuotasYAML(t *testing.T) {
	type QuotasDesc struct {
		Quotas []string
	}
	for _, name := range resourceToQuotaName {
		t.Run(name, func(t *testing.T) {
			quotasFile := yatest.SourcePath("cloud/bitbucket/private-api/yandex/cloud/priv/mdb/v2/quotas.yaml")
			buf, err := ioutil.ReadFile(quotasFile)
			require.NoError(t, err)
			var quotas QuotasDesc
			err = yaml.Unmarshal(buf, &quotas)
			require.NoError(t, err, "failed to parse quotas.yaml")

			require.Contains(t, quotas.Quotas, name)
		})
	}
}
