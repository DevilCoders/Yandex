package move

import (
	"a.yandex-team.ru/cloud/compute/go-common/util"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"golang.org/x/net/context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const objectCheckInterval = 1 * time.Minute

type urlMoveSrc struct {
	nms *nbdMoveSrc

	getCurrentEtag misc.GetEtag
	etag           string

	m           sync.Mutex
	closed      bool
	closedError error
}

func (ums *urlMoveSrc) GetBlockSize() int64 {
	return ums.nms.GetBlockSize()
}

func (ums *urlMoveSrc) GetSize() int64 {
	return ums.nms.GetSize()
}

func (ums *urlMoveSrc) ReadAt(ctx context.Context, offset int64, data []byte) (exists, zero bool, err error) {
	g := util.MakeLockGuard(&ums.m)
	g.Lock()
	defer g.UnlockIfLocked()
	if ums.closed {
		return false, false, ums.closedError
	}
	g.Unlock()

	if offset%ums.GetBlockSize() != 0 {
		return false, false, misc.ErrInvalidOffset
	}

	if int64(len(data))%ums.GetBlockSize() != 0 {
		return false, false, misc.ErrInvalidBlockSize
	}

	_, _, err = ums.nms.ReadAt(ctx, offset, data)
	if err != nil {
		return false, false, err
	}

	var nz chunker.NotZero
	_, _ = nz.Write(data)
	return true, !bool(nz), nil
}

func (ums *urlMoveSrc) GetHint() directreader.Hint {
	return ums.nms.GetHint()
}

func (ums *urlMoveSrc) Close(ctx context.Context, err error) error {
	ums.m.Lock()
	defer ums.m.Unlock()

	if !ums.closed {
		ums.closed = true
		ums.closedError = ums.nms.Close(ctx, err)

		if tag, err := ums.getCurrentEtag(); err != nil || tag != ums.etag {
			if err == nil {
				err = misc.ErrSourceChanged
			}
			if ums.closedError != nil {
				ums.closedError = err
			}
		}

	}

	return ums.closedError
}

func (ums *urlMoveSrc) closeChanged() bool {
	g := util.MakeLockGuard(&ums.m)

	g.Lock()
	defer g.UnlockIfLocked()
	if ums.closed {
		return true
	}
	g.Unlock()

	if tag, err := ums.getCurrentEtag(); err != nil || tag != ums.etag {
		if err == nil {
			err = misc.ErrSourceChanged
		}

		g.Lock()
		ums.closed = true
		if ums.closedError != nil {
			ums.closedError = err
		}
		return true
	}
	return false
}

func (ums *urlMoveSrc) objectChecker() {
	ticker := time.NewTicker(objectCheckInterval)
	for range ticker.C {
		if ums.closeChanged() {
			break
		}
	}
	ticker.Stop()
}

func (mdf *DeviceFactory) newURLMoveSrc(ctx context.Context, src *common.URLMoveSrc, blocksMap directreader.BlocksMap, getEtag misc.GetEtag) (Src, error) {
	img, err := mdf.reg.Get(ctx, &common.ConvertRequest{
		Format: src.Format,
		Bucket: src.Bucket,
		Key:    src.Key,
		URL:    src.ShadowedURL,
	}, blocksMap)
	if err != nil {
		return nil, err
	}

	nbdParams, err := mdf.c.GetNbdParams(ctx, img)
	if err != nil {
		return nil, err
	}
	client := &urlMoveSrc{nms: newNbdMoveSrc(nbdParams), getCurrentEtag: getEtag, etag: blocksMap.GetEtag()}

	go client.objectChecker()

	return client, nil
}
