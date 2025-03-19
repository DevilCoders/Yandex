package directreader

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/url"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"cuelang.org/go/pkg/strconv"
	"golang.org/x/xerrors"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	noMapErrorMessage       = "qemu-img: File contains external, encrypted or compressed clusters."
	qemuImgTimeout          = 15 * time.Second
	qemuImgDefaultReadahead = 4 * misc.MB
)

// http://manpages.ubuntu.com/manpages/trusty/man1/qemu-img.1.html (map [-f fmt] [--output=ofmt] filename)
type imageMapItem struct {
	Start          int64  `json:"start"`  // Offset in logical disk inside image
	Length         int64  `json:"length"` // Length block in bytes
	Depth          int64  `json:"depth"`  // Depth of images (file which contain raw block)
	Zero           bool   `json:"zero"`   // Block contain only zeroes
	Data           bool   `json:"data"`   // Block contain data
	RawImageOffset *int64 `json:"offset"` // Offset in file for direct read raw data. Pointer need for detect unexisted fields after json decode.
}

func (a imageMapItem) HasNonZeroContent() bool {
	return a.Data && !a.Zero && a.RawImageOffset != nil
}

func (a imageMapItem) SameContentTypeTo(b imageMapItem) bool {
	return a.Data == b.Data && a.Zero == b.Zero
}

func (a imageMapItem) IsPrevTo(b imageMapItem) bool {
	return b.Start == a.Start+a.Length
}

func (a imageMapItem) RawIsPrevTo(b imageMapItem) bool {
	return !a.HasNonZeroContent() || !b.HasNonZeroContent() || *b.RawImageOffset == *a.RawImageOffset+a.Length
}

// BlocksMap contain map of data blocks between logical offset and raw offset in image file.
type BlocksMap struct {
	items []imageMapItem
	url   string

	etag string
}

func FromJSON(raw []byte, url string) (blk BlocksMap, err error) {
	blk.url = url
	err = json.Unmarshal(raw, &blk.items)
	if err != nil {
		return
	}
	// no documentation about order of mapped blocks
	// ensure about order by start
	sort.Slice(blk.items, func(i, j int) bool {
		return blk.items[i].Start < blk.items[j].Start
	})
	return blk, err
}

func FromHuman(raw []byte, url string) (blk BlocksMap, err error) {
	blk.url = url
	data := bytes.FieldsFunc(raw, func(r rune) bool {
		return r == rune('\n')
	})
	if len(data) >= 2 && strings.HasPrefix(string(data[1]), noMapErrorMessage) {
		return BlocksMap{}, xerrors.Errorf("no map present: %w", misc.ErrIncorrectMap)
	}
	if len(data) <= 1 {
		return BlocksMap{}, xerrors.Errorf("block map is empty: %w", misc.ErrIncorrectMap)
	}
	data = data[1:]
	blk.items = make([]imageMapItem, len(data))
	for i, str := range data {
		liters := bytes.FieldsFunc(str, func(r rune) bool {
			return r == rune(' ')
		})
		start, err := strconv.ParseInt(string(liters[0]), 0, 64)
		if err != nil {
			return BlocksMap{}, xerrors.Errorf("failed to parse start: %w", err)
		}
		length, err := strconv.ParseInt(string(liters[1]), 0, 64)
		if err != nil {
			return BlocksMap{}, xerrors.Errorf("failed to parse length: %w", err)
		}
		offset, err := strconv.ParseInt(string(liters[2]), 0, 64)
		if err != nil {
			return BlocksMap{}, xerrors.Errorf("failed to parse offset: %w", err)
		}
		blk.items[i].Data = true
		blk.items[i].Depth = 0
		blk.items[i].Zero = false
		blk.items[i].Start = start
		blk.items[i].Length = length
		blk.items[i].RawImageOffset = &offset
	}
	// no documentation about order of mapped blocks
	// ensure about order by start
	sort.Slice(blk.items, func(i, j int) bool {
		return blk.items[i].Start < blk.items[j].Start
	})
	return blk, nil
}

func (m BlocksMap) CountBlocksWithData() (n int64) {
	for _, item := range m.items {
		if item.Data {
			n++
		}
	}
	return
}

func (m BlocksMap) VirtualSize() int64 {
	if len(m.items) == 0 {
		return 0
	}
	return m.items[len(m.items)-1].Start + m.items[len(m.items)-1].Length
}

// Size return logical size of image, presented as map
func (m BlocksMap) Size() int64 {
	if len(m.items) == 0 {
		return 0
	}

	lastItem := m.items[len(m.items)-1]
	return lastItem.Start + lastItem.Length
}

func (m BlocksMap) GetEtag() string {
	return m.etag
}

// String return url of source image of blockmap - for human readable.
func (m BlocksMap) String() string {
	return m.url
}

// Check about map is corrent: contain mapping from zero to last block
func (m BlocksMap) check(fileSize int64) bool {
	if len(m.items) == 0 {
		return false
	}

	nextStart := int64(0)
	for _, item := range m.items {
		if item.Start != nextStart {
			return false
		}
		nextStart = item.Start + item.Length
		// blocks jumped out of file
		if item.RawImageOffset != nil && (*item.RawImageOffset+item.Length > fileSize || item.Length == 0) {
			return false
		}
	}
	return true
}

func (m *BlocksMap) CanReadDirect() bool {
	for _, item := range m.items {
		if item.Data && !item.Zero && item.RawImageOffset == nil || item.Depth != 0 {
			// Block is data and non-zero but have no real offset
			return false
		}
	}
	return true
}

func (m *BlocksMap) MergeAdjacentBlocks() {
	if len(m.items) == 0 || !m.CanReadDirect() {
		return
	}

	newItems := make([]imageMapItem, len(m.items))
	currentBlockPosition := 0
	currentBlock := m.items[0]
	for _, b := range m.items[1:] {
		if currentBlock.SameContentTypeTo(b) && currentBlock.IsPrevTo(b) && currentBlock.RawIsPrevTo(b) {
			currentBlock.Length += b.Length
		} else {
			newItems[currentBlockPosition] = currentBlock
			currentBlockPosition++
			currentBlock = b
		}
	}

	newItems[currentBlockPosition] = currentBlock
	currentBlockPosition++

	m.items = newItems[:currentBlockPosition] // no +1 here, Check on len(items) == 1
}

// ReadBlocksMap read map blocks of image.
func ReadBlocksMap(ctx context.Context, url string, format string, getEtag misc.GetEtag, getLength misc.GetLength, proxySock string) (resBlockMap BlocksMap, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	t := misc.ReadBlockMapTimer.Start(ctx)
	defer t.ObserveDuration()

	ctx, ctxCancel := context.WithCancel(ctx)
	defer func() {
		log.G(ctx).Debug("Canceling ctx due to end ReadBlocksMap")
		ctxCancel()
	}()

	logger := log.G(ctx)
	logger.Debug("Read blocks map of image", zap.String("url", url), zap.String("format", format))

	args := []string{"qemu-img", "map", "--output=json"}
	infoArgs := []string{"qemu-img", "info"}
	if format != "" {
		args = append(args, "-f", format)
		infoArgs = append(args, "-f", format)
	}

	blk, err := CreateQemuJSONTarget(url, qemuImgDefaultReadahead, qemuImgTimeout)
	if err != nil {
		return BlocksMap{}, fmt.Errorf("failed to create json target for qemu: %w", err)
	}

	args = append(args, blk)
	infoArgs = append(infoArgs, blk)

	_ = misc.Retry(ctx, "qemu-img info", func() error {
		info, err := dockerprocess.Exec(ctx, infoArgs, map[string]string{proxySock: dockerprocess.ProxySocketInContainer})
		if err == nil {
			logger.Debug("Qemu-img info finished", zap.ByteString("output", info))
		}

		return nil
	})

	var (
		blockMap   BlocksMap
		errInRetry error
		rawMap     []byte
	)
	_ = misc.Retry(ctx, "qemu-img read map", func() error {
		logger.Debug("Start command qemu-img", zap.Strings("cmd", args))

		rawMap, errInRetry = dockerprocess.Exec(ctx, args, map[string]string{proxySock: dockerprocess.ProxySocketInContainer})
		log.DebugError(logger, errInRetry, "Read qemu block map")
		if errInRetry != nil {
			// Too long output means map itself is too big to be processed by snapshot
			if errors.Is(errInRetry, misc.ErrTooLongOuput) {
				errInRetry = misc.ErrTooFragmented
				return errInRetry
			}
			return misc.ErrInternalRetry
		}

		blockMap, errInRetry = FromJSON(rawMap, url)
		log.DebugError(logger, errInRetry, "Parse qemu block map")
		if errInRetry != nil {
			return misc.ErrInternalRetry
		}
		misc.ReadBlockMapBytes.ObserveContext(ctx, float64(len(rawMap)))
		return nil
	})

	if errInRetry != nil {
		return BlocksMap{}, errInRetry
	}

	if blockMap.etag, err = getEtag(); err != nil {
		return BlocksMap{}, err
	}
	length, err := getLength()
	if err != nil {
		return BlocksMap{}, err
	}

	// raw image case, qemu returned block size rounded by 512bytes, so we need to set block size explicitly from request
	if len(blockMap.items) == 1 && format == "raw" {
		blockMap.items[0].Start = 0
		blockMap.items[0].Length = length
	}

	if !blockMap.check(length) {
		logger.Error("Read incorrect map",
			zap.Reflect("map_items", blockMap.items), zap.Int64("file_size", length))
		return BlocksMap{}, misc.ErrIncorrectMap
	}

	blockMap.MergeAdjacentBlocks()
	misc.ReadBlockMapItems.ObserveContext(ctx, float64(len(blockMap.items)))

	if !blockMap.check(length) {
		logger.Error("Read incorrect map",
			zap.Reflect("map_items", blockMap.items), zap.Int64("file_size", length))
		return BlocksMap{}, misc.ErrIncorrectMap
	}

	return blockMap, nil
}

type blockdev struct {
	Driver    string `json:"file.driver"`
	URL       string `json:"file.url"`
	Readahead int    `json:"file.readahead"`
	Timeout   int    `json:"file.timeout"`
}

func CreateQemuJSONTarget(rawURL string, readahead int, timeout time.Duration) (string, error) {
	u, err := url.Parse(rawURL)
	if err != nil {
		return "", err
	}

	if u.Scheme == "" {
		// At least we tried
		u.Scheme = "http"
	}
	buf := bytes.NewBuffer([]byte("json:"))
	err = json.NewEncoder(buf).Encode(
		blockdev{
			Driver:    u.Scheme,
			URL:       u.String(),
			Readahead: readahead,
			Timeout:   int(timeout / time.Second),
		})
	if err != nil {
		return "", err
	}
	res := buf.Bytes()
	for i, ch := range res {
		if ch == '"' {
			res[i] = '\''
		}
	}
	res = bytes.TrimSpace(res)
	return string(res), nil
}
