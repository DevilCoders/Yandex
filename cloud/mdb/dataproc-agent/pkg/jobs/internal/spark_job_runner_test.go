package internal

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

func TestSparkJobGetCommand(t *testing.T) {
	jobRunner := &SparkJobRunner{
		id: "999",
		SparkJob: models.SparkJob{
			MainJarFileURI: "WordCount.jar",
			MainClass:      "WordCount",
			JarFileURIs:    []string{"file1.jar", "file2.jar"},
			FileURIs:       []string{"file1.txt", "file2.txt"},
			ArchiveURIs:    []string{"archive1.zip", "archive2.zip"},
			Packages: []string{"org.apache.spark:spark-streaming-kafka_2.10:1.6.0",
				"org.elasticsearch:elasticsearch-spark_2.10:2.2.0"},
			Repositories: []string{"https://oss.sonatype.org/content/groups/public/",
				"https://oss.sonatype.org/content/groups/public/"},
			ExcludePackages: []string{"org.apache.spark:spark-streaming-kafka_2.10"},
			Properties:      map[string]string{"key1": "val1", "key2": "val2"},
			Args:            []string{"arg1", "arg2"},
		},
	}
	err := jobRunner.BeforeRun()
	require.Equal(t, nil, err)

	targetCommand := "spark-submit" +
		" --class WordCount" +
		" --archives archive1.zip,archive2.zip" +
		" --exclude-packages org.apache.spark:spark-streaming-kafka_2.10" +
		" --files file1.txt,file2.txt" +
		" --jars file1.jar,file2.jar" +
		" --packages org.apache.spark:spark-streaming-kafka_2.10:1.6.0,org.elasticsearch:elasticsearch-spark_2.10:2.2.0" +
		" --repositories https://oss.sonatype.org/content/groups/public/,https://oss.sonatype.org/content/groups/public/" +
		" --conf key1=val1" +
		" --conf key2=val2" +
		" --conf spark.submit.deployMode=cluster" +
		" --conf spark.yarn.tags=dataproc_job_999" +
		" WordCount.jar" +
		" arg1 arg2"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestSparkJobGetCommandWithDeployModeClient(t *testing.T) {
	jobRunner := &SparkJobRunner{
		id: "999",
		SparkJob: models.SparkJob{
			MainJarFileURI: "main.jar",
			Properties:     map[string]string{"spark.submit.deployMode": "client"},
		},
	}
	err := jobRunner.BeforeRun()
	require.Equal(t, nil, err)

	targetCommand := "spark-submit" +
		" --conf spark.submit.deployMode=client" +
		" --conf spark.yarn.tags=dataproc_job_999" +
		" main.jar"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestSparkJobGetCommandWithExtraTag(t *testing.T) {
	jobRunner := &SparkJobRunner{
		id: "999",
		SparkJob: models.SparkJob{
			MainJarFileURI: "main.jar",
			Properties:     map[string]string{"spark.yarn.tags": "tag1"},
		},
	}
	err := jobRunner.BeforeRun()
	require.Equal(t, nil, err)

	targetCommand := "spark-submit" +
		" --conf spark.submit.deployMode=cluster" +
		" --conf spark.yarn.tags=tag1,dataproc_job_999" +
		" main.jar"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}

func TestSparkJobGetCommandWithEmptyProperties(t *testing.T) {
	jobRunner := &SparkJobRunner{
		id: "999",
		SparkJob: models.SparkJob{
			MainJarFileURI: "main.jar",
		},
	}
	err := jobRunner.BeforeRun()
	require.Equal(t, nil, err)

	targetCommand := "spark-submit" +
		" --conf spark.submit.deployMode=cluster" +
		" --conf spark.yarn.tags=dataproc_job_999" +
		" main.jar"
	require.Equal(t, targetCommand, buildCommandLine(jobRunner))
}
