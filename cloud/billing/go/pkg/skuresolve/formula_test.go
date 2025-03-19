package skuresolve

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type skuFormulaTestSuite struct {
	suite.Suite
	metric string
	getter MetricValueGetter
}

func TestSkuFormula(t *testing.T) {
	suite.Run(t, new(skuFormulaTestSuite))
}

func (suite *skuFormulaTestSuite) SetupTest() {
	suite.metric = `
{
	"schema": "schema",
	"version": "version",
	"usage":{
		"quantity": 2,
		"start": 999,
		"finish": 1000,
		"unit": "unit",
		"type": "delta"
	},
	"tags":{
		"cores": 3,
		"size": 999.85,
		"disk_size": 100500,
		"gpus": 8,
		"memory": 2048000000000,
		"software_accelerated_network_cores": 16,
		"ydb_size": 2048,
		"ydb_storage_size": 2048000000000.5,
		"public_ip": 1,
		"transferred": "500",
		"nan": "this is not number"
	}
}
`
	fg, err := ParseToFastJSONGetter(suite.metric)
	suite.Require().NoError(err)
	suite.getter = fg
}

func (suite *skuFormulaTestSuite) TestFormulas() {
	cases := []struct {
		formula string
		want    decimal.Decimal128
	}{
		{"`1`", decimal.Must(decimal.FromString("1"))},
		{"tags.cores", decimal.Must(decimal.FromString("3"))},
		{"mul(mul(tags.cores, usage.quantity), tags.size)", decimal.Must(decimal.FromString("5999.1"))},
		{"mul(mul(tags.memory, usage.quantity), tags.size)", decimal.Must(decimal.FromString("4095385600000000"))},
		{"mul(tags.cores, usage.quantity)", decimal.Must(decimal.FromString("6"))},
		{"mul(tags.disk_size, usage.quantity)", decimal.Must(decimal.FromString("201000"))},
		{"mul(tags.gpus, usage.quantity)", decimal.Must(decimal.FromString("16"))},
		{"mul(tags.memory, usage.quantity)", decimal.Must(decimal.FromString("4096000000000"))},
		{"mul(tags.size, usage.quantity)", decimal.Must(decimal.FromString("1999.7"))},
		{"mul(tags.software_accelerated_network_cores, usage.quantity)", decimal.Must(decimal.FromString("32"))},
		{"mul(tags.ydb_size, usage.quantity)", decimal.Must(decimal.FromString("4096"))},
		{"mul(tags.ydb_storage_size, usage.quantity)", decimal.Must(decimal.FromString("4096000000001"))},
		{"mul(usage.quantity, tags.public_ip)", decimal.Must(decimal.FromString("2"))},
		{"quantum(max([tags.cores, `4`]), `2`)", decimal.Must(decimal.FromString("4"))},
		{"quantum(max([tags.cores, `2`]), `2`)", decimal.Must(decimal.FromString("4"))},
		{"tags.transferred", decimal.Must(decimal.FromString("500"))},
		{"usage.quantity", decimal.Must(decimal.FromString("2"))},
		{"usage.start", decimal.Must(decimal.FromString("999"))},
		{"usage.finish", decimal.Must(decimal.FromString("1000"))},
		{"usage.unit == 'unit' && '1'", decimal.Must(decimal.FromString("1"))},
		{"usage.type == 'delta' && '1'", decimal.Must(decimal.FromString("1"))},
		{"usage.finish", decimal.Must(decimal.FromString("1000"))},
		{"schema == 'schema' && '1'", decimal.Must(decimal.FromString("1"))},
		{"version == 'version' && '1'", decimal.Must(decimal.FromString("1"))},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.formula), func() {
			got, err := ApplyFormula(JMESPath(c.formula), suite.getter)
			suite.Require().NoError(err)
			suite.Zero(c.want.Cmp(got), got.String())
		})
	}
}

func (suite *skuFormulaTestSuite) TestParseError() {
	_, err := ApplyFormula(JMESPath("'"), suite.getter)
	suite.Require().Error(err)
}

func (suite *skuFormulaTestSuite) TestExecError() {
	_, err := ApplyFormula(JMESPath("add('type error', tags.exists)"), suite.getter)
	suite.Require().Error(err)
}

func (suite *skuFormulaTestSuite) TestResultInvalidType() {
	_, err := ApplyFormula(JMESPath("schema"), suite.getter)
	suite.Require().Error(err)
}

func (suite *skuFormulaTestSuite) TestNanResult() {
	_, err := ApplyFormula(JMESPath("tags.nan"), suite.getter)
	suite.Require().Error(err)
}
