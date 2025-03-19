package functest

import (
	"io/ioutil"

	"github.com/DATA-DOG/godog/gherkin"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
)

func (tctx *testContext) logsdbResponse(data *gherkin.DocString) error {
	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("logsdb.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	return tctx.LogsDBCtx.FillData([]byte(data.Content), false)
}

func (tctx *testContext) namedLogsdbResponse(data *gherkin.DocString) error {
	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath("logsdb.json"), []byte(data.Content), 0666); err != nil {
		return err
	}

	return tctx.LogsDBCtx.FillData([]byte(data.Content), true)
}
