package common

import (
	"context"
	"fmt"
	"io"
	"net/http"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func HTTPGetBody(
	ctx context.Context,
	httpClient *http.Client,
	url string,
	start, end int64, // Closed interval [start:end].
	etag string,
) (reader io.ReadCloser, err error) {

	requestedContentLength := end - start + 1
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return nil, &task_errors.RetriableError{
			Err: err,
		}
	}

	if end < start || start < 0 {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"bad start-end range for http request: [%d:%d]",
				start,
				end,
			),
		}
	}

	reqCtx, cancelReqCtx := context.WithCancel(context.Background())

	req = req.WithContext(reqCtx)
	defer func() {
		if err != nil {
			logging.Debug(
				ctx,
				"Canceling ctx due to error in httpGetBody in imagereader: %v",
				err,
			)
			cancelReqCtx()
		}
	}()

	req.Header.Set("RANGE", fmt.Sprintf("bytes=%v-%v", start, end))
	var resp *http.Response

	// TODO: try use http2 streams https://st.yandex-team.ru/NBS-3253
	resp, err = httpClient.Do(req)
	if err != nil {
		return nil, &task_errors.RetriableError{
			Err: fmt.Errorf(
				"error while http request range [%d:%d]: %v",
				start,
				end,
				err,
			),
		}
	}

	// Retry temporary client errors.
	if resp.StatusCode == http.StatusTooManyRequests ||
		resp.StatusCode == http.StatusLocked ||
		resp.StatusCode == http.StatusRequestTimeout {
		return nil, &task_errors.RetriableError{
			Err: fmt.Errorf(
				"client temporary error while http range request: code %d",
				resp.StatusCode,
			),
		}
	}

	if resp.StatusCode == http.StatusRequestedRangeNotSatisfiable {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"requested range [%d:%d] doesn't satisfiable",
				start,
				end,
			),
		}
	}

	if resp.StatusCode >= 400 && resp.StatusCode <= 499 {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"client error while http range [%d:%d] request: code %d, status: %v",
				start,
				end,
				resp.StatusCode,
				resp.Status,
			),
		}
	}

	// Retry server errors.
	if resp.StatusCode >= 500 && resp.StatusCode <= 599 {
		return nil, &task_errors.RetriableError{
			Err: fmt.Errorf(
				"server error while http range request: code %d",
				resp.StatusCode,
			),
		}
	}

	if resp.Header.Get("Etag") != etag {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"wrong etag found: wanted %v, found %v",
				etag,
				resp.Header.Get("Etag"),
			),
		}
	}

	if resp.ContentLength != requestedContentLength {
		return nil, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"server return bad length of data: requested %d, actual %d",
				requestedContentLength,
				resp.ContentLength,
			),
		}
	}

	return httpReadCloser{
		cancelCtx:    cancelReqCtx,
		ioReadCloser: resp.Body,
	}, nil
}

type httpReadCloser struct {
	cancelCtx    func()
	ioReadCloser io.ReadCloser
}

func (r httpReadCloser) Read(p []byte) (int, error) {
	return r.ioReadCloser.Read(p)
}

func (r httpReadCloser) Close() error {
	r.cancelCtx()
	return r.ioReadCloser.Close()
}
