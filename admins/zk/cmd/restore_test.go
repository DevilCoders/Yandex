package main

import (
	"io/ioutil"
	"os"
	"testing"

	"a.yandex-team.ru/admins/zk"
	"a.yandex-team.ru/admins/zk/mocks"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
)

func init() {
	loggerConfig.OutputPaths = []string{"stdout"}
	logger = zap.Must(loggerConfig)
	loggerConfig.Level.SetLevel(zap.ZapifyLevel(log.TraceLevel))
}

func TestRestore(t *testing.T) {
	ctrl := gomock.NewController(t)

	var dumpData = []byte(`
{"Path":"/music","Data":null}
{"Path":"/music/testing","Data":null}
{"Path":"/music/testing/dyn-properties","Data":"aGk="}
{"Path":"/music/testing/for","Data":"ZnJvbQ=="}
{"Path":"/music/testing/test","Data":"Y2hlY2s="}
{"Path":"/zookeeper/config","Data":null}
{"Path":"/filtered/nodes","Data":null}
{"Path":"/filtered/out","Data":null}
{"Path":"/filtered/in","Data":null}
`)
	dumpJSON, err := ioutil.TempFile(".", "dump.json")
	defer os.Remove(dumpJSON.Name())
	assert.NoError(t, err)
	assert.NoError(t, ioutil.WriteFile(dumpJSON.Name(), dumpData, 0600))

	var zMockClient = mocks.NewMockClient(ctrl)

	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/").Times(1)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/music").Times(1)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/music/testing").Times(3)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/filtered").MaxTimes(1)

	_ = zMockClient.EXPECT().Create(nil, "/music", nil, gomock.Any(), gomock.Any()).Return("", zk.ErrNodeExists)
	_ = zMockClient.EXPECT().Set(nil, "/music", nil, gomock.Any()).Times(0)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing", nil, gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/dyn-properties", []byte("hi"), gomock.Any(), gomock.Any()).Return("", zk.ErrNodeExists)
	_ = zMockClient.EXPECT().Set(nil, "/music/testing/dyn-properties", []byte("hi"), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/for", []byte("from"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/test", []byte("check"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/filtered/in", nil, gomock.Any(), gomock.Any()).Times(1)

	var cmdRestoreForTest = cmdRestore(zMockClient)
	assert.NoError(t, cmdRestoreForTest.Flag("input").Value.Set(dumpJSON.Name()))
	assert.NoError(t, cmdRestoreForTest.Flag("filter").Value.Set("/(music|zookeeper|filtered/in)"))
	assert.NoError(t, cmdRestoreForTest.Flag("parallel").Value.Set("0")) // expected to be fixed to 1 or restore stuck
	assert.NotPanics(t, func() { cmdRestoreForTest.Run(cmdRestoreForTest, []string{"/"}) })
}

func TestRestoreParallel(t *testing.T) {
	ctrl := gomock.NewController(t)

	var dumpData = []byte(`
{"Path":"/music","Data":null}
{"Path":"/music/testing","Data":null}
{"Path":"/music/testing/dyn-properties","Data":"aGk="}
{"Path":"/music/testing/for","Data":"ZnJvbQ=="}
{"Path":"/music/testing/test","Data":"Y2hlY2s="}
{"Path":"/zookeeper/config","Data":null}
{"Path":"/nodes/1","Data":"MQo="}
{"Path":"/nodes/2","Data":"Mgo="}
{"Path":"/nodes/3","Data":"Mwo="}
{"Path":"/nodes/4","Data":"NAo="}
{"Path":"/nodes/5","Data":"NQo="}
{"Path":"/nodes/6","Data":"Ngo="}
`)
	dumpJSON, err := ioutil.TempFile(".", "dump.json")
	defer os.Remove(dumpJSON.Name())
	assert.NoError(t, err)
	assert.NoError(t, ioutil.WriteFile(dumpJSON.Name(), dumpData, 0600))

	var zMockClient = mocks.NewMockClient(ctrl)

	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/").Times(1)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/music").Times(1)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/music/testing").Times(3)
	_ = zMockClient.EXPECT().EnsureZkPathCached(nil, "/nodes").Times(6)

	_ = zMockClient.EXPECT().Create(nil, "/music", nil, gomock.Any(), gomock.Any()).Return("", zk.ErrNodeExists)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing", nil, gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/dyn-properties", []byte("hi"), gomock.Any(), gomock.Any()).Return("", zk.ErrNodeExists)
	_ = zMockClient.EXPECT().Set(nil, "/music/testing/dyn-properties", []byte("hi"), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/for", []byte("from"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/music/testing/test", []byte("check"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/1", []byte("1\n"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/2", []byte("2\n"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/3", []byte("3\n"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/4", []byte("4\n"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/5", []byte("5\n"), gomock.Any(), gomock.Any()).Times(1)
	_ = zMockClient.EXPECT().Create(nil, "/nodes/6", []byte("6\n"), gomock.Any(), gomock.Any()).Times(1)

	var cmdRestoreForTest = cmdRestore(zMockClient)
	assert.NoError(t, cmdRestoreForTest.Flag("input").Value.Set(dumpJSON.Name()))
	assert.NoError(t, cmdRestoreForTest.Flag("parallel").Value.Set("4"))
	assert.NotPanics(t, func() { cmdRestoreForTest.Run(cmdRestoreForTest, []string{"/"}) })
}
