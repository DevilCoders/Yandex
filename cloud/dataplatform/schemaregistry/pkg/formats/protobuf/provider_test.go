package protobuf_test

import (
	"bytes"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"math/rand"
	"os/exec"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/pkg/formats/protobuf"
)

func runProtoc(
	rootDir string,
	includeImports bool,
	descSetOut string,
) error {
	protocBinPath, err := exec.LookPath("protoc")
	if err != nil {
		return err
	}
	protocBinPath, err = filepath.EvalSymlinks(protocBinPath)
	if err != nil {
		return err
	}
	protocBinPath, err = filepath.Abs(protocBinPath)
	if err != nil {
		return err
	}
	protocIncludePath, err := filepath.Abs(filepath.Join(filepath.Dir(protocBinPath), "..", "include"))
	if err != nil {
		return err
	}
	args := []string{"-I", rootDir, "-I", protocIncludePath}
	args = append(args, fmt.Sprintf("--descriptor_set_out=%s", descSetOut))
	if includeImports {
		args = append(args, "--include_imports")
	}
	protoFiles, _ := filepath.Glob(filepath.Join(rootDir, "./**/*.proto"))
	rootFiles, _ := filepath.Glob(filepath.Join(rootDir, "./*.proto"))
	args = append(args, rootFiles...)
	args = append(args, protoFiles...)
	stderr := bytes.NewBuffer(nil)
	cmd := exec.Command(protocBinPath, args...)
	cmd.Stdout = stderr
	cmd.Stderr = stderr
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("%s returned error: %v %v", protocBinPath, err, stderr.String())
	}
	return nil
}

func getRandomName() string {
	b := make([]byte, 10)
	rand.Read(b)
	return hex.EncodeToString(b)
}

func TestGetParsedSchema(t *testing.T) {
	t.Run("should return error if protobuf data is not valid", func(t *testing.T) {
		data := []byte("invalid data")
		_, err := protobuf.GetParsedSchema(data)
		assert.Error(t, err)
	})
	t.Run("should be able to parse valid protobuf data", func(t *testing.T) {
		data := getDescriptorData(t, "./testdata/valid", true)
		parsedSchema, err := protobuf.GetParsedSchema(data)
		assert.NoError(t, err)
		testM := map[string]interface{}{
			"data": data,
		}
		jsonD, _ := json.Marshal(testM)
		logger.Log.Infof("enc: %s", jsonD)
		logger.Log.Infof("enc: %v", data)
		testR := map[string][]byte{}
		assert.NoError(t, json.Unmarshal(jsonD, &testR))
		parsedSchema, err = protobuf.GetParsedSchema(testR["data"])
		assert.NoError(t, err)
		assert.NotNil(t, parsedSchema)
	})
	t.Run("should return error if protobuf data is not fully contained", func(t *testing.T) {
		data := getDescriptorData(t, "./testdata/valid", false)
		_, err := protobuf.GetParsedSchema(data)
		assert.Error(t, err)
	})
}
