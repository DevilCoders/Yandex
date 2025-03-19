package client

import (
	"bytes"
	"context"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/timestamp"
	"github.com/stretchr/testify/assert"

	intapi "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/v1"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/test/assertpb"
)

func convertTime(t *testing.T, jsonTime string) *timestamp.Timestamp {
	tm, err := time.Parse(time.RFC3339, jsonTime)
	if err != nil {
		assert.FailNowf(t, "Failed to parse time string", err.Error())
	}
	createdAt, err := ptypes.TimestampProto(tm)
	if err != nil {
		assert.FailNowf(t, "Failed to convert time string to protobuf type", err.Error())
	}
	return createdAt
}

func TestListClusterJobs(t *testing.T) {
	testCases := []struct {
		name           string
		request        *intapi.ListJobsRequest
		handler        http.HandlerFunc
		expectedResult *intapi.ListJobsResponse
		expectedError  error
	}{
		{
			"all request params are passed correctly",
			&intapi.ListJobsRequest{
				ClusterId: "777",
				PageSize:  10,
				PageToken: "ttt",
				Filter:    "fff",
			},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				expectedURL := "/mdb/hadoop/1.0/jobs?cluster_id=777&filters=fff&pageSize=10&pageToken=ttt"
				assert.Equal(t, expectedURL, r.URL.String())
				_, _ = w.Write([]byte(`{"jobs": []}`))
			}),
			&intapi.ListJobsResponse{Jobs: []*intapi.Job{}},
			nil,
		},
		{
			"deserializes main job params and all job statuses",
			&intapi.ListJobsRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"jobs": [
						{
							"id":"job1",
							"clusterId":"777",
							"name":"some job name",
							"status":"PROVISIONING",
							"createdBy":"user777",
							"unknownField":"value",
							"createdAt":"2019-10-21T12:00:10.243Z",
							"startedAt":null,
							"finishedAt":null
						},
						{
							"id":"job2",
							"clusterId":"777",
							"status":"PENDING",
							"createdAt":"2019-10-21T12:01:10.243Z",
							"startedAt":"2019-10-21T12:02:10.243Z"
						},
						{
							"id":"job3",
							"clusterId":"777",
							"status":"RUNNING"
						}
					],
					"next_page_token": "next_token"
				}`))
			}),
			&intapi.ListJobsResponse{
				Jobs: []*intapi.Job{
					{
						Id:         "job1",
						ClusterId:  "777",
						Name:       "some job name",
						Status:     intapi.Job_PROVISIONING,
						CreatedBy:  "user777",
						CreatedAt:  convertTime(t, "2019-10-21T12:00:10.243Z"),
						StartedAt:  nil,
						FinishedAt: nil,
					},
					{
						Id:        "job2",
						ClusterId: "777",
						Status:    intapi.Job_PENDING,
						CreatedAt: convertTime(t, "2019-10-21T12:01:10.243Z"),
						StartedAt: convertTime(t, "2019-10-21T12:02:10.243Z"),
					},
					{
						Id:        "job3",
						ClusterId: "777",
						Status:    intapi.Job_RUNNING,
					},
				},
				NextPageToken: "next_token",
			},
			nil,
		},
		{
			"deserializes spark jobs",
			&intapi.ListJobsRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"jobs": [
						{
							"sparkJob":{
								"args": ["arg1", "arg2"],
								"fileUris": ["file1.txt", "file2.txt"],
								"mainClass": "SpecialWordCount",
								"properties": {"key1": "val1", "key2": "val2"},
								"archiveUris": ["archive1.zip", "archive2.zip"],
								"jarFileUris": ["file1.jar", "file2.jar"],
								"mainJarFileUri": "s3a://bucket/WordCount.jar"
							}
						}
					]
				}`))
			}),
			&intapi.ListJobsResponse{
				Jobs: []*intapi.Job{
					{
						JobSpec: &intapi.Job_SparkJob{SparkJob: &intapi.SparkJob{
							Args:           []string{"arg1", "arg2"},
							JarFileUris:    []string{"file1.jar", "file2.jar"},
							FileUris:       []string{"file1.txt", "file2.txt"},
							ArchiveUris:    []string{"archive1.zip", "archive2.zip"},
							Properties:     map[string]string{"key1": "val1", "key2": "val2"},
							MainJarFileUri: "s3a://bucket/WordCount.jar",
							MainClass:      "SpecialWordCount",
						}},
					},
				},
			},
			nil,
		},
		{
			"deserializes pyspark jobs",
			&intapi.ListJobsRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"jobs": [
						{
							"pysparkJob": {
								"args": ["arg1", "arg2"],
								"archiveUris": ["archive1.zip", "archive2.zip"],
								"fileUris": ["file1.txt", "file2.txt"],
								"jarFileUris": ["file1.jar", "file2.jar"],
								"pythonFileUris": ["file1.py", "file2.py"],
								"properties": {"key1": "val1", "key2": "val2"},
								"mainPythonFileUri": "main.py"
							}
						}
					]
				}`))
			}),
			&intapi.ListJobsResponse{
				Jobs: []*intapi.Job{
					{
						JobSpec: &intapi.Job_PysparkJob{PysparkJob: &intapi.PysparkJob{
							Args:              []string{"arg1", "arg2"},
							ArchiveUris:       []string{"archive1.zip", "archive2.zip"},
							FileUris:          []string{"file1.txt", "file2.txt"},
							JarFileUris:       []string{"file1.jar", "file2.jar"},
							PythonFileUris:    []string{"file1.py", "file2.py"},
							Properties:        map[string]string{"key1": "val1", "key2": "val2"},
							MainPythonFileUri: "main.py",
						}},
					},
				},
			},
			nil,
		},
		{
			"deserializes mapreduce jobs",
			&intapi.ListJobsRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"jobs": [
						{
							"mapreduceJob": {
								"args": ["-mapper", "mapper.py", "-reducer", "reducer.py"],
								"archiveUris": ["archive1.zip", "archive2.zip"],
								"fileUris": ["s3a://bucket/mapper.py", "s3a://bucket/reducer.py"],
								"jarFileUris": ["file1.jar", "file2.jar"],
								"properties": {"key1": "val1", "key2": "val2"},
								"mainJarFileUri": "hadoop-streaming.jar"
							}
						},
						{
							"mapreduceJob": {
								"mainClass": "WordCount"
							}
						}
					]
				}`))
			}),
			&intapi.ListJobsResponse{
				Jobs: []*intapi.Job{
					{
						JobSpec: &intapi.Job_MapreduceJob{MapreduceJob: &intapi.MapreduceJob{
							Args:        []string{"-mapper", "mapper.py", "-reducer", "reducer.py"},
							ArchiveUris: []string{"archive1.zip", "archive2.zip"},
							FileUris:    []string{"s3a://bucket/mapper.py", "s3a://bucket/reducer.py"},
							JarFileUris: []string{"file1.jar", "file2.jar"},
							Properties:  map[string]string{"key1": "val1", "key2": "val2"},
							Driver:      &intapi.MapreduceJob_MainJarFileUri{MainJarFileUri: "hadoop-streaming.jar"},
						}},
					},
					{
						JobSpec: &intapi.Job_MapreduceJob{MapreduceJob: &intapi.MapreduceJob{
							Driver: &intapi.MapreduceJob_MainClass{MainClass: "WordCount"},
						}},
					},
				},
			},
			nil,
		},
		{
			"deserializes hive jobs",
			&intapi.ListJobsRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"jobs": [
						{
							"hiveJob":{
								"continueOnFailure": true,
								"properties": {"prop1": "val3", "prop2": "val4"},
								"scriptVariables": {"var1": "val1", "var2": "val2"},
								"jarFileUris": ["file1.jar", "file2.jar"],
								"queryFileUri": "script.sql"
							}
						},
						{
							"hiveJob":{
								"queryList": {
									"queries": ["query1", "query2"]
								}
							}
						}
					]
				}`))
			}),
			&intapi.ListJobsResponse{
				Jobs: []*intapi.Job{
					{
						JobSpec: &intapi.Job_HiveJob{HiveJob: &intapi.HiveJob{
							ContinueOnFailure: true,
							ScriptVariables:   map[string]string{"var1": "val1", "var2": "val2"},
							Properties:        map[string]string{"prop1": "val3", "prop2": "val4"},
							JarFileUris:       []string{"file1.jar", "file2.jar"},
							QueryType:         &intapi.HiveJob_QueryFileUri{QueryFileUri: "script.sql"},
						}},
					},
					{
						JobSpec: &intapi.Job_HiveJob{HiveJob: &intapi.HiveJob{
							QueryType: &intapi.HiveJob_QueryList{QueryList: &intapi.QueryList{
								Queries: []string{"query1", "query2"},
							}},
						}},
					},
				},
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ts := httptest.NewServer(tc.handler)
			client, err := New(Config{URL: ts.URL}, &nop.Logger{})
			assert.NoError(t, err)
			response, err := client.ListClusterJobs(context.Background(), tc.request)

			if tc.expectedError == nil {
				assert.NoError(t, err)
				assertpb.Equal(t, tc.expectedResult, response)
			} else {
				assert.EqualError(t, err, tc.expectedError.Error())
			}
			ts.Close()
		})
	}
}

func TestUpdateJobStatus(t *testing.T) {
	testCases := []struct {
		name           string
		request        *intapi.UpdateJobStatusRequest
		handler        http.HandlerFunc
		expectedResult *intapi.UpdateJobStatusResponse
		expectedError  error
	}{
		{
			"all request params are passed correctly",
			&intapi.UpdateJobStatusRequest{
				ClusterId: "777",
				JobId:     "666",
				Status:    intapi.Job_DONE,
			},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				expectedURL := "/mdb/hadoop/1.0/clusters/777/jobs/666:updateStatus"
				assert.Equal(t, expectedURL, r.URL.String())
				assert.Equal(t, "PATCH", r.Method)

				buf := new(bytes.Buffer)
				_, _ = buf.ReadFrom(r.Body)
				assert.Equal(t, `{"cluster_id":"777","job_id":"666","status":"DONE"}`, buf.String())
				_, _ = w.Write([]byte(`{}`))
			}),
			&intapi.UpdateJobStatusResponse{},
			nil,
		},
		{
			"response is parsed correctly",
			&intapi.UpdateJobStatusRequest{},
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"clusterId": "111",
					"job_id": "222",
					"status": "ERROR"
				}`))
			}),
			&intapi.UpdateJobStatusResponse{
				ClusterId: "111",
				JobId:     "222",
				Status:    intapi.Job_ERROR,
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ts := httptest.NewServer(tc.handler)
			client, err := New(Config{URL: ts.URL}, &nop.Logger{})
			assert.NoError(t, err)
			response, err := client.UpdateJobStatus(context.Background(), tc.request)

			if tc.expectedError == nil {
				assert.NoError(t, err)
				assertpb.Equal(t, tc.expectedResult, response)
			} else {
				assert.EqualError(t, err, tc.expectedError.Error())
			}
			ts.Close()
		})
	}
}
