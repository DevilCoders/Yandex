package internal

import (
	"testing"

	"github.com/stretchr/testify/require"

	hadoopfs "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/hadoop/fs"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestMapreduceJobGetCommand(t *testing.T) {
	jobRunner := &MapreduceJobRunner{
		id: "999",
		MapreduceJob: models.MapreduceJob{
			MainJarFileURI: "hadoop-streaming.jar",
			MainClass:      "",
			JarFileURIs:    []string{"file1.jar", "file2.jar"},
			FileURIs:       []string{"s3a://bucket/mapper.py", "s3a://bucket/reducer.py"},
			ArchiveURIs:    []string{"archive1.zip", "archive2.zip"},
			Properties:     map[string]string{"key1": "val1", "key2": "val2"},
			Args:           []string{"ClassName", "-mapper", "mapper.py", "-reducer", "reducer.py"},
		},
	}

	targetCommand := "yarn" +
		" jar hadoop-streaming.jar ClassName" +
		" -Dkey1=val1" +
		" -Dkey2=val2" +
		" -Dmapreduce.job.tags=dataproc_job_999" +
		" -archives archive1.zip,archive2.zip" +
		" -files s3a://bucket/mapper.py,s3a://bucket/reducer.py" +
		" -libjars file1.jar,file2.jar" +
		" -mapper mapper.py -reducer reducer.py"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestMapreduceJobMainClassFromJar(t *testing.T) {
	jobRunner := &MapreduceJobRunner{
		id: "999",
		MapreduceJob: models.MapreduceJob{
			MainJarFileURI: "job.jar",
			Args:           []string{"arg1", "arg2"},
		},
		mainClassFromJar: "MainClass",
	}

	targetCommand := "yarn" +
		" jar job.jar" +
		" -Dmapreduce.job.tags=dataproc_job_999 arg1 arg2"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestMapreduceExamples(t *testing.T) {
	jobRunner := &MapreduceJobRunner{
		id: "999",
		MapreduceJob: models.MapreduceJob{
			MainJarFileURI: "/usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar",
			Args:           []string{"pi", "1", "1000"},
		},
		mainClassFromJar: "org.apache.hadoop.examples.ExampleDriver",
	}

	targetCommand := "yarn" +
		" jar /usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar pi" +
		" -Dmapreduce.job.tags=dataproc_job_999 1 1000"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestMapreduceJobGetCommandWithMainClass(t *testing.T) {
	jobRunner := &MapreduceJobRunner{
		id: "999",
		MapreduceJob: models.MapreduceJob{
			MainJarFileURI: "",
			MainClass:      "ClassFromJar",
			JarFileURIs:    []string{"jar_with_class.jar"},
		},
	}

	targetCommand := "yarn" +
		" ClassFromJar" +
		" -Dmapreduce.job.tags=dataproc_job_999" +
		" -libjars jar_with_class.jar"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestMapreduceJobGetCommandWithExtraTag(t *testing.T) {
	jobRunner := &MapreduceJobRunner{
		id: "999",
		MapreduceJob: models.MapreduceJob{
			MainClass:  "MainClass",
			Properties: map[string]string{"mapreduce.job.tags": "tag1"},
		},
	}

	targetCommand := "yarn" +
		" MainClass" +
		" -Dmapreduce.job.tags=tag1,dataproc_job_999"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestDownloadFile(t *testing.T) {
	defer func() { copyToLocal = hadoopfs.CopyToLocal }()

	// success
	copyToLocal = func(sourceURL string, localPath string) error {
		require.Equal(t, "s3a://bucket-name/file.jar", sourceURL)
		require.Equal(t, "/tmp/job_001/file.jar", localPath)
		return nil
	}
	localPath, err := downloadFile("s3a://bucket-name/file.jar", "/tmp/job_001", &nop.Logger{})
	require.NoError(t, err)
	require.Equal(t, "/tmp/job_001/file.jar", localPath)

	// error
	copyToLocal = func(sourceURL string, localPath string) error {
		require.Equal(t, "https://yandex.ru/file.jar", sourceURL)
		require.Equal(t, "/tmp/job_002/file.jar", localPath)
		return xerrors.New("404 Not Found")
	}
	_, err = downloadFile("https://yandex.ru/file.jar", "/tmp/job_002", &nop.Logger{})
	require.Error(t, err)
	require.Equal(t, "failed to download file https://yandex.ru/file.jar: 404 Not Found", err.Error())

	// local file with scheme file://
	copyToLocal = func(sourceURL string, localPath string) error {
		t.Fatal("this should not be called")
		return nil
	}
	localPath, err = downloadFile("file:///usr/lib/file.jar", "/tmp/job_003", &nop.Logger{})
	require.NoError(t, err)
	require.Equal(t, "file:///usr/lib/file.jar", localPath)

	// local file
	copyToLocal = func(sourceURL string, localPath string) error {
		t.Fatal("this should not be called")
		return nil
	}
	localPath, err = downloadFile("/usr/lib/file.jar", "/tmp/job_004", &nop.Logger{})
	require.NoError(t, err)
	require.Equal(t, "/usr/lib/file.jar", localPath)
}
