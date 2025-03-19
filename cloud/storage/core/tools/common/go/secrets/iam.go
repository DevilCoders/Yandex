package secrets

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

////////////////////////////////////////////////////////////////////////////////

type ycpClient struct {
	logutil.WithLog

	path string
}

func (y *ycpClient) readAll(
	ctx context.Context,
	pipe io.Reader,
) ([]string, error) {

	var output []string

	scanner := bufio.NewScanner(pipe)
	scanner.Split(bufio.ScanLines)

	for scanner.Scan() {
		output = append(output, scanner.Text())
	}

	return output, scanner.Err()
}

func (y *ycpClient) createIAMTokenImpl(
	ctx context.Context,
	profileName string,
) (string, error) {

	command := exec.CommandContext(
		ctx,
		y.path,
		"--profile", profileName,
		"iam",
		"create-token",
	)

	errPipe, err := command.StderrPipe()
	if err != nil {
		return "", fmt.Errorf("StderrPipe: %w", err)
	}

	outPipe, err := command.StdoutPipe()
	if err != nil {
		return "", fmt.Errorf("StdoutPipe: %w", err)
	}

	if err := command.Start(); err != nil {
		return "", fmt.Errorf("can't start ycp: %w", err)
	}

	stderr, err := y.readAll(ctx, errPipe)
	if err != nil {
		return "", fmt.Errorf("can't read errors from pipe: %w", err)
	}

	stdout, err := y.readAll(ctx, outPipe)
	if err != nil {
		return "", fmt.Errorf("can't read data from pipe: %w", err)
	}

	if len(stderr) != 0 {
		if !strings.HasPrefix(stderr[0], "There is a new ycp version") {
			return "", fmt.Errorf("unexpected errors: %v", stderr)
		}

		y.LogWarn(ctx, "[YCP] %v", strings.Join(stderr, "\n"))
	}

	if len(stdout) != 1 {
		return "", fmt.Errorf("unexpected output: %v", stdout)
	}

	return stdout[0], nil
}

func (y *ycpClient) createIAMToken(
	ctx context.Context,
	config *YcpConfig,
	clusterName string,
) (string, error) {
	var profileName string

	if config != nil {
		profileName = config.Profiles[clusterName]
	}

	if len(profileName) == 0 {
		y.LogDbg(
			ctx,
			"[YCP] can't find profile for cluster '%v' in ycp configs. Use cluster name.",
			clusterName,
		)

		profileName = clusterName
	}

	y.LogInfo(ctx, "[YCP] create IAM token...")

	token, err := y.createIAMTokenImpl(ctx, profileName)
	if err != nil {
		y.LogError(
			ctx,
			"[YCP] can't create IAM token: %v",
			err,
		)
	} else {
		y.LogInfo(ctx, "[YCP] create IAM token: OK")
	}

	return token, err
}

////////////////////////////////////////////////////////////////////////////////

func CreateIAMToken(
	ctx context.Context,
	log nbs.Log,
	config *YcpConfig,
	clusterName string,
	ycpPath string,
) (string, error) {
	ycp := &ycpClient{
		WithLog: logutil.WithLog{
			Log: log,
		},
		path: ycpPath,
	}

	return ycp.createIAMToken(ctx, config, clusterName)
}

func TryGetIAMToken(
	ctx context.Context,
	log nbs.Log,
	config *YcpConfig,
	clusterName string,
	ycpPath string,
) (string, error) {

	token := os.Getenv("IAM_TOKEN")
	if len(token) != 0 {
		return token, nil
	}

	token, err := CreateIAMToken(ctx, log, config, clusterName, ycpPath)

	if err != nil {
		return "", fmt.Errorf("can't create IAM token: %w", err)
	}

	return token, nil
}
