package agent

import (
	"bufio"
	"context"
	"io"
	"net"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// FetchZKInfo fetches and parses info from zookeper server
func FetchZKInfo(ctx context.Context, url string) (models.ZookeeperInfo, error) {
	var d net.Dialer
	conn, err := d.DialContext(ctx, "tcp", url)
	if err != nil {
		return models.ZookeeperInfo{}, xerrors.Errorf("failed to connect to zookeeper: %w", err)
	}

	deadline, ok := ctx.Deadline()
	if !ok {
		return models.ZookeeperInfo{}, xerrors.Errorf("can use only context with deadline: %w", err)
	}

	err = conn.SetWriteDeadline(deadline)
	if err != nil {
		return models.ZookeeperInfo{}, xerrors.Errorf("can't set write deadline: %w", err)
	}
	_, err = conn.Write([]byte("ruok"))
	if err != nil {
		return models.ZookeeperInfo{}, xerrors.Errorf("failed to send command to zookeeper: %w", err)
	}

	err = conn.SetReadDeadline(deadline)
	if err != nil {
		return models.ZookeeperInfo{}, xerrors.Errorf("can't set read deadline: %w", err)
	}
	resp, err := bufio.NewReader(conn).ReadString('\n')
	if err != nil && err != io.EOF {
		return models.ZookeeperInfo{}, xerrors.Errorf("failed to read response from zookeeper: %w", err)
	}
	if resp != "imok" {
		return models.ZookeeperInfo{}, xerrors.Errorf("wrong response from zookeeper %q: %w", resp, err)
	}

	return models.ZookeeperInfo{Available: true}, nil
}
