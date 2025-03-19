package functest

import (
	"bufio"
	"io/ioutil"
	"os"
	"strings"
	"time"

	apihelpers "a.yandex-team.ru/cloud/mdb/dbaas-internal-api-image/recipe/helpers"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (tctx *testContext) weCreateFlag(name string) error {
	if err := os.MkdirAll(apihelpers.MustTmpRootPath(""), 0777); err != nil {
		return err
	}

	if err := ioutil.WriteFile(apihelpers.MustTmpRootPath(name), []byte(""), 0666); err != nil {
		return err
	}

	return nil
}

func (tctx *testContext) stringIsNotInTskvLog(str string) error {
	return checkFileForString(apihelpers.MustLogsRootPath("internal-api.tskv"), str, false)
}

func (tctx *testContext) stringIsInTskvLog(str string) error {
	return checkFileForString(apihelpers.MustLogsRootPath("internal-api.tskv"), str, true)
}

func checkFileForString(filename, str string, exists bool) error {
	file, err := os.Open(filename)
	if err != nil {
		return xerrors.Errorf("failed to open internal api tskv log: %w", err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		if !strings.Contains(scanner.Text(), str) {
			continue
		}

		// Found unexpected string
		if !exists {
			return xerrors.New("log contains unexpected string")
		}

		// Found expected string
		return nil
	}

	if err = scanner.Err(); err != nil {
		return xerrors.Errorf("scanner error: %w", err)
	}

	// Did not find expected string
	if exists {
		return xerrors.New("log does not contain expected string")
	}

	// Did not find unexpected string
	return nil
}

func (tctx *testContext) stepISaveValAs(val, varname string) error {
	tctx.variables[varname] = val
	return nil
}

func (tctx *testContext) stepISaveNowAs(varname string) error {
	tctx.variables[varname] = time.Now()
	return nil
}
