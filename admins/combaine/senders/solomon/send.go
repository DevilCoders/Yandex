package solomon

import (
	"bytes"
	"context"
	"fmt"
	"net/url"
	"sync"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/admins/combaine/senders"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

func (t *TaskConfig) payloadType() string {
	typ := "json"
	if t.useSpack {
		typ = "spack"
		if t.useSpackCompression {
			typ += "+lz4"
		}
	}
	return typ
}

func (t *TaskConfig) getURLParams() string {
	q := url.Values{}
	q.Add("project", t.Project)
	q.Add("cluster", t.Cluster)
	q.Add("service", t.Service)
	return q.Encode()
}

// Send parse data, build and send http request to solomon api
func (t *TaskConfig) Send(task *senders.SenderTask) error {
	if len(task.Data) == 0 {
		t.L.Warn("Empty data. Nothing to send")
		return nil
	}

	var merr []error
	var wg sync.WaitGroup
	var errCh = make(chan error)

	pusher := resty.New().
		SetHeaders(map[string]string{
			"User-Agent": "arcadia/admins/combaine/solomon",
			"Accept":     "application/json",
		}).
		SetQueryParams(map[string]string{
			"project": t.Project,
			"cluster": t.Cluster,
			"service": t.Service,
		}).
		SetAuthScheme("OAuth").
		SetAuthToken(t.token).
		SetHeader("Content-Type", "application/json")

	if t.useSpack {
		pusher.SetHeader("Content-Type", "application/x-solomon-spack")
		if t.useSpackCompression {
			pusher.SetHeader("Content-Encoding", "lz4")
		}
	}

	for _, item := range task.Data {
		host, err := senders.GetSubgroupName(item.Tags)
		if err != nil {
			merr = append(merr, fmt.Errorf("skip task: %w", err))
			continue
		}

		reg, err := t.dumpSensors(host, item)
		if err != nil {
			merr = append(merr, err)
			continue
		}

		t.L.Debugf("Send sensors for: %s&%s", host, t.getURLParams())
		wg.Add(1)
		go t.send(host, pusher, reg, &wg, errCh)
	}
	go func() {
		wg.Wait()
		close(errCh)
	}()
	for err := range errCh {
		merr = append(merr, err)
	}
	if len(merr) > 0 {
		return fmt.Errorf("%d errors occurred: %v", len(merr), merr)
	}
	return nil
}

func (t *TaskConfig) send(host string, pusher *resty.Client, reg *solomon.Registry, wg *sync.WaitGroup, c chan error) {
	defer wg.Done()
	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(t.Timeout)*time.Millisecond)
	defer cancel()

	var err error
	var n int
	var body bytes.Buffer
	req := pusher.NewRequest()

	if t.useSpack {
		compression := solomon.CompressionNone
		if t.useSpackCompression {
			compression = solomon.CompressionLz4
		}
		n, err = reg.StreamSpack(ctx, &body, compression)
	} else {
		n, err = reg.StreamJSON(ctx, &body)
	}
	var resp *resty.Response
	if err == nil {
		resp, err = req.SetBody(body.Bytes()).
			SetContext(ctx).
			Post(t.API)

		if err == nil && resp.IsError() {
			err = fmt.Errorf("bad status code %d: %s", resp.StatusCode(), string(resp.Body()))
		}

	}
	if err != nil {
		c <- fmt.Errorf("host: %s, writen=%d bytes: %w", host, n, err)
		t.L.Debugf("Failed to send for host=%s writen=%d bytes: %v", host, n, err)
	} else {
		t.L.Infof("Successfully sent sensors (%d bytes in %s) for: host=%s&%s: %s", n, t.payloadType(), host, t.getURLParams(), resp.String())
	}
}
