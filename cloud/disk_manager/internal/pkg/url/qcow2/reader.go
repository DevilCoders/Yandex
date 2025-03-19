package qcow2

/* this code is a very light-weight version of original qemu-img:
https://github.com/qemu/qemu/blob/master/block/qcow2.c
- do not support external files
- do not support subclusters
- do not support crypto
*/

import (
	"context"
	"encoding/binary"
	"fmt"
	"math/bits"
	"net/http"
	"unsafe"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	task_errors "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url/mapitem"
)

////////////////////////////////////////////////////////////////////////////////

type QCOW2Reader struct {
	url        string
	httpClient *http.Client
	size       uint64
	etag       string

	// QCOW2 image data.
	header                header
	clusterBits           uint32
	clusterSize           uint32
	l2Size                uint32
	l2Bits                uint32
	l2EntrySize           uint32
	l2SliceSize           uint32
	l2Cache               map[uint64][]uint64
	l1Table               []uint64
	clusterOffsetMask     uint64
	csizeShift            uint32
	csizeMask             uint64
	hasSubclusters        bool
	subclusterSize        uint32
	subclusterBits        uint32
	subclustersPerCluster uint32
}

func (r *QCOW2Reader) Size() uint64 {
	return r.header.Size
}

func (r *QCOW2Reader) ReadHeader(
	ctx context.Context,
) (bool, error) {

	err := r.read(ctx, 0, uint64(unsafe.Sizeof(r.header)), &r.header)
	if err != nil {
		logging.Debug(ctx, "Fail to read qcow2 header: %v", err)
		return false, nil
	}

	if checkQCOW2Magic(r.header.Magic) {
		logging.Debug(ctx, "Fail to check qcow2 magic: expected - %v, actual - %v", qcow2Magic, r.header.Magic)
		return false, nil
	}

	if r.header.Version < version2 || r.header.Version > version3 {
		logging.Debug(ctx, "Fail to check qcow2 version: version %d", r.header.Version)
		return false, nil
	}

	if r.header.CryptMethod != cryptNone {
		return true, &task_errors.NonRetriableError{
			Err: fmt.Errorf("do not support crypto qcow2: cryptMethod: %v", r.header.CryptMethod),
		}
	}

	r.clusterBits = r.header.ClusterBits
	r.clusterSize = 1 << r.clusterBits

	r.l2EntrySize = 8
	if r.header.IncompatibleFeatures&(1<<4) != 0 {
		r.l2EntrySize *= 2
	}

	r.l2Size = r.clusterSize / r.l2EntrySize
	r.l2Bits = uint32(bits.TrailingZeros32(r.l2Size))

	r.subclustersPerCluster = 1
	if r.header.IncompatibleFeatures&(1<<4) != 0 {
		r.subclustersPerCluster = 32
	}

	r.subclusterSize = r.clusterSize / r.subclustersPerCluster
	r.subclusterBits = uint32(bits.TrailingZeros32(r.subclusterSize))

	r.hasSubclusters = r.header.IncompatibleFeatures&(1<<4) != 0
	if r.hasSubclusters {
		return true, &task_errors.NonRetriableError{
			Err: fmt.Errorf("do not support subsclusters in qcow2"),
		}
	}

	r.l2SliceSize = r.clusterSize / r.l2EntrySize

	r.clusterOffsetMask = (1 << (63 - r.clusterBits)) - 1
	r.csizeShift = 62 - (r.clusterBits - 8)
	r.csizeMask = (1 << (r.clusterBits - 8)) - 1

	return true, nil
}

func (r *QCOW2Reader) clusterType(l2Entry uint64) qCow2ClusterType {
	if l2Entry&qcowOflagCompressed != 0 {
		return qcow2ClusterCompressed
	}

	if !r.hasSubclusters && l2Entry&qcowOflagZero != 0 {
		if l2Entry&l2OffsetMask != 0 {
			return qcow2ClusterZeroAlloc
		}
		return qcow2ClusterZeroPlain
	}

	if !(l2Entry&l2OffsetMask != 0) {
		return qcow2ClusterUnallocated
	}

	return qcow2ClusterNormal
}

func (r *QCOW2Reader) subclusterType(l2Entry uint64) qCow2SubclusterType {
	switch r.clusterType(l2Entry) {
	case qcow2ClusterCompressed:
		return qcow2SubclusterCompressed
	case qcow2ClusterZeroPlain:
		return qcow2SubclusterZeroPlain
	case qcow2ClusterZeroAlloc:
		return qcow2SubclusterZeroAlloc
	case qcow2ClusterNormal:
		return qcow2SubclusterNormal
	case qcow2ClusterUnallocated:
		return qcow2SubclusterUnallocatedPlain
	}

	return qcow2SubclusterInvalid
}

func (r *QCOW2Reader) countContiguousSubclusters(
	nbClusters uint64,
	scIndex uint,
	l2Slice []uint64,
	l2Index uint64,
) (int, error) {

	var expectedType qCow2SubclusterType
	var checkOffset bool
	var expectedOffset uint64

	count := 0

	if l2Index+nbClusters > uint64(r.l2SliceSize) {
		return 0, &task_errors.NonRetriableError{
			Err: fmt.Errorf(
				"subcluster index out of l2 slice range: l2Index %d, nbClusters %d, l2SSlizeSize %d",
				l2Index,
				nbClusters,
				r.l2SliceSize),
		}
	}

	for i := uint64(0); i < nbClusters; i++ {
		firstSc := uint(0)
		if i == 0 {
			firstSc = scIndex
		}

		l2Idx := (l2Index + i) * uint64(r.l2EntrySize) / 8
		l2Entry := l2Slice[l2Idx]

		subType := r.subclusterType(l2Entry)
		ret, err := r.qcow2GetSubclusterRangeType(l2Entry, uint32(firstSc))
		if err != nil {
			return 0, err
		}

		if i == 0 {
			if subType == qcow2SubclusterCompressed {
				return ret, nil
			}

			expectedType = subType
			expectedOffset = l2Entry & l2OffsetMask
			checkOffset = subType == qcow2SubclusterNormal ||
				subType == qcow2SubclusterZeroAlloc ||
				subType == qcow2SubclusterUnallocatedAlloc
		} else if subType != expectedType {
			break
		} else if checkOffset {
			expectedOffset += uint64(r.clusterSize)
			if expectedOffset != (l2Entry & l2OffsetMask) {
				break
			}
		}

		count += ret

		if firstSc+uint(ret) < uint(r.subclustersPerCluster) {
			break
		}
	}

	return count, nil
}

func (r *QCOW2Reader) sizeToClusters(size uint64) uint64 {
	return (size + uint64(r.subclusterSize-1)) >> r.subclusterBits
}

func (r *QCOW2Reader) offsetToL2SliceIndex(offset uint64) uint64 {
	return (offset >> r.clusterBits) & uint64(r.l2SliceSize-1)
}

func (r *QCOW2Reader) offsetToL2Index(offset uint64) uint64 {
	return (offset >> r.clusterBits) & (uint64(r.l2Size) - 1)
}

func (r *QCOW2Reader) offsetIntoCluster(offset uint64) uint64 {
	return offset & uint64(r.clusterSize-1)
}

func (r *QCOW2Reader) qcow2GetSubclusterRangeType(
	l2Entry uint64,
	scFrom uint32,
) (int, error) {

	subType := r.subclusterType(l2Entry)

	if subType == qcow2SubclusterInvalid {
		return 0, &task_errors.NonRetriableError{
			Err: fmt.Errorf("invalid subType: %v", subType),
		}
	} else {
		return int(r.subclustersPerCluster - scFrom), nil
	}
}

func (r *QCOW2Reader) l2Table(
	ctx context.Context,
	offset uint64,
	l2Offset uint64,
) ([]uint64, error) {

	startOfSlice := uint64(r.l2EntrySize) * (r.offsetToL2Index(offset) - r.offsetToL2SliceIndex(offset))

	realOffset := l2Offset + startOfSlice

	value, ok := r.l2Cache[realOffset]
	if ok {
		return value, nil
	}

	l2Table := make([]uint64, r.clusterSize/8)

	err := r.read(ctx, realOffset, uint64(len(l2Table))*8, &l2Table)
	if err != nil {
		return nil, err
	}

	r.l2Cache[realOffset] = l2Table

	return l2Table, nil
}

func (r *QCOW2Reader) read(
	ctx context.Context,
	start, count uint64,
	data interface{},
) error {

	end := start + count - 1

	body, err := common.HTTPGetBody(
		ctx,
		r.httpClient,
		r.url,
		int64(start),
		int64(end),
		r.etag,
	)
	if err != nil {
		return err
	}
	defer body.Close()

	err = binary.Read(body, binary.BigEndian, data)
	// NBS-3324: interpret all errors as retriable.
	return task_errors.MakeRetriable(err, false /* ignoreRetryLimit */)
}

type mapEntry struct {
	clusterStatus qlusterStatus
	mapOffset     uint64
	coffset       uint64
	csize         uint64
}

func (m *mapEntry) mergeable(entry mapEntry) bool {
	return m.clusterStatus == entry.clusterStatus &&
		m.mapOffset == entry.mapOffset &&
		m.coffset == 0 &&
		m.csize == 0
}

func (m *mapEntry) dumpToItem(item *mapitem.ImageMapItem) {
	if m.clusterStatus == clusterData {
		item.Zero = false
		item.Data = true
	} else {
		item.Zero = true
		item.Data = false
	}

	if m.mapOffset != 0 {
		item.RawImageOffset = new(int64)
		*item.RawImageOffset = int64(m.mapOffset)
	}

	if m.coffset != 0 {
		item.CompressedOffset = new(uint64)
		*item.CompressedOffset = m.coffset
		item.CompressedSize = new(uint64)
		*item.CompressedSize = m.csize
	}
}

func (r *QCOW2Reader) readChunk(
	ctx context.Context,
	offset uint64,
	bytes uint64,
) (uint64, mapEntry, error) {

	entry := mapEntry{
		clusterStatus: clusterUnallocated,
		mapOffset:     0,
	}

	l1Index := offset >> (r.l2Bits + r.clusterBits)
	l2Offset := r.l1Table[l1Index] & l1OffsetMask
	l2Index := (offset >> r.clusterBits) & (uint64(r.l2Size) - 1)
	offsetInCluster := offset & (uint64(r.clusterSize) - 1)

	scIndex := (offset >> r.subclusterBits) & (uint64(r.subclustersPerCluster) - 1)

	bytesNeeded := bytes + offsetInCluster
	bytesAvailable := (uint64(r.l2SliceSize) - r.offsetToL2SliceIndex(offset)) << r.clusterBits

	if bytesNeeded > bytesAvailable {
		bytesNeeded = bytesAvailable
	}

	nbClusters := r.sizeToClusters(bytesNeeded)

	if l2Offset == 0 {
		return bytesAvailable - offsetInCluster, entry, nil
	}

	l2Table, err := r.l2Table(ctx, offset, l2Offset)
	if err != nil {
		return 0, mapEntry{}, err
	}

	l2Entry := l2Table[l2Index]

	subType := r.subclusterType(l2Entry)

	sc, err := r.countContiguousSubclusters(nbClusters, uint(scIndex), l2Table, l2Index)
	if err != nil {
		return 0, mapEntry{}, err
	}
	bytesAvailable = (uint64(sc) + scIndex) << r.subclusterBits
	if bytesAvailable > bytesNeeded {
		bytesAvailable = bytesNeeded
	}

	switch subType {
	case qcow2SubclusterInvalid:
		break
	case qcow2SubclusterCompressed:
		entry.coffset = l2Entry & r.clusterOffsetMask
		nbCSectors := ((l2Entry >> r.csizeShift) & r.csizeMask) + 1
		entry.csize = nbCSectors*qcow2CompressedSectorSize - (entry.coffset & (qcow2CompressedSectorSize - 1))
	case qcow2SubclusterZeroPlain, qcow2SubclusterUnallocatedPlain:
		break
	case qcow2SubclusterZeroAlloc, qcow2SubclusterNormal, qcow2SubclusterUnallocatedAlloc:
		hostClusterOffset := l2Entry & l2OffsetMask
		entry.mapOffset = hostClusterOffset + offsetInCluster
		if r.offsetIntoCluster(hostClusterOffset) != 0 {
			return 0, mapEntry{}, &task_errors.NonRetriableError{
				Err: fmt.Errorf(
					"cluster allocation offset %d unaligned (L2 offset: %d, L2 index %d)",
					hostClusterOffset,
					l2Offset,
					l2Index),
			}
		}
	}

	if subType == qcow2SubclusterZeroPlain || subType == qcow2SubclusterZeroAlloc {
		entry.clusterStatus = clusterZero
	} else if subType != qcow2SubclusterUnallocatedPlain && subType != qcow2SubclusterUnallocatedAlloc {
		entry.clusterStatus = clusterData
	}

	return bytesAvailable - offsetInCluster, entry, nil
}

func (r *QCOW2Reader) ReadImageMap(
	ctx context.Context,
) (<-chan mapitem.ImageMapItem, <-chan error) {

	items := make(chan mapitem.ImageMapItem)
	errors := make(chan error, 1)

	r.l1Table = make([]uint64, r.header.L1Size)
	err := r.read(ctx, r.header.L1TableOffset, uint64(r.header.L1Size)*8, &r.l1Table)
	if err != nil {
		errors <- err
		close(items)
		close(errors)
		return items, errors
	}

	go func() {
		defer close(items)
		defer close(errors)

		item := mapitem.ImageMapItem{
			Start:  0,
			Length: 0,
		}

		entry := mapEntry{
			clusterStatus: clusterUnallocated,
		}

		for item.Start+item.Length < int64(r.header.Size) {
			offset := item.Start + item.Length
			n := int64(r.header.Size) - offset

			bytesRead, newEntry, err := r.readChunk(ctx, uint64(offset), uint64(n))
			if err != nil {
				errors <- err
				return
			}

			if entry.mergeable(newEntry) {
				item.Length += int64(bytesRead)
			} else {
				if item.Length != 0 {
					entry.dumpToItem(&item)
					select {
					case items <- item:
					case <-ctx.Done():
						errors <- ctx.Err()
						return
					}
				}

				item = mapitem.ImageMapItem{
					Start:  offset,
					Length: int64(bytesRead),
				}

				entry = newEntry
			}
		}

		entry.dumpToItem(&item)
		select {
		case items <- item:
		case <-ctx.Done():
			errors <- ctx.Err()
			return
		}
	}()

	return items, errors
}

func NewQCOW2Reader(
	url string,
	httpClient *http.Client,
	size uint64,
	etag string,
) *QCOW2Reader {

	return &QCOW2Reader{
		url:        url,
		httpClient: httpClient,
		size:       size,
		etag:       etag,
		l2Cache:    make(map[uint64][]uint64),
	}
}
