package internal

import (
	"testing"

	iowrap "github.com/spf13/afero"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
)

func init() {
	FS = iowrap.NewMemMapFs()
	FSUtil = &iowrap.Afero{Fs: FS}
}

func TestHiveJobGetCommand(t *testing.T) {
	hiveJob := &HiveJobRunner{
		id: "999",
		HiveJob: models.HiveJob{
			QueryFileURI:      "script.sql",
			ContinueOnFailure: true,
			ScriptVariables:   map[string]string{"var1": "val1", "var2": "val2"},
			Properties:        map[string]string{"prop1": "val3", "prop2": "val4"},
			JarFileURIs:       []string{"file1.jar", "file2.jar"},
		},
	}
	hiveJob.queryFilePath = "/tmp/hive-query-2158977262/query_with_jars.hql"

	targetCommand := "beeline -u jdbc:hive2://localhost:10000" +
		" --force=true" +
		" --hiveconf hive.session.id=dataproc_job_999" +
		" --hiveconf prop1=val3" +
		" --hiveconf prop2=val4" +
		" --hiveconf tez.application.tags=dataproc_job_999" +
		" --hivevar var1=val1" +
		" --hivevar var2=val2" +
		" -f /tmp/hive-query-2158977262/query_with_jars.hql"
	require.Equal(t, targetCommand, buildCommandLine(hiveJob))
}

func TestHiveJobGetCommandWithQueries(t *testing.T) {
	hiveJob := &HiveJobRunner{
		id: "999",
		HiveJob: models.HiveJob{
			QueryList: []string{"query1", "query2"},
		},
	}
	hiveJob.queryFilePath = "/tmp/hive-query-2158977262/query_with_jars.hql"

	targetCommand := "beeline -u jdbc:hive2://localhost:10000" +
		" --hiveconf hive.session.id=dataproc_job_999" +
		" --hiveconf tez.application.tags=dataproc_job_999" +
		" -f /tmp/hive-query-2158977262/query_with_jars.hql"
	require.Equal(t, targetCommand, buildCommandLine(hiveJob))
}

func TestHiveJobSelectYarnApplication(t *testing.T) {
	jobID := "777"
	realApp := yarn.Application{Name: "HIVE-dataproc_job_" + jobID, State: yarn.StateRunning}
	apps := []yarn.Application{realApp}
	job := &HiveJobRunner{
		id: jobID,
		HiveJob: models.HiveJob{
			QueryFileURI: "script.sql",
		},
	}

	app, err := job.SelectYarnApplication(apps)
	require.NoError(t, err)
	require.Equal(t, realApp, *app)
}

func TestBeforeRun(t *testing.T) {
	hiveJob := &HiveJobRunner{
		id: "999",
		HiveJob: models.HiveJob{
			QueryList: []string{
				"SELECT a FROM b;",
				"SELECT b FROM c;",
			},
			ContinueOnFailure: true,
			ScriptVariables:   map[string]string{"var1": "val1", "var2": "val2"},
			Properties:        map[string]string{"prop1": "val3", "prop2": "val4"},
			JarFileURIs:       []string{"file1.jar", "file2.jar"},
		},
	}
	err := hiveJob.BeforeRun()
	require.NoError(t, err)
	require.NotNil(t, hiveJob.queryFilePath)
	fsbuf, _ := iowrap.ReadFile(FS, hiveJob.queryFilePath)
	expected := "ADD JAR file1.jar;\n" + "ADD JAR file2.jar;\n" + "SELECT a FROM b;;\n" + "SELECT b FROM c;;\n"
	require.Equal(t, expected, string(fsbuf))
}
