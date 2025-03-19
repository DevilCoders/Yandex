package qcow2

// Represents a header of qcow2 image format.
type header struct {
	Magic                 uint32      //     [0:3] qcow2Magic: QCOW qcow2Magic string ("QFI\xfb").
	Version               version     //     [4:7] Version number.
	BackingFileOffset     uint64      //    [8:15] Offset into the image file at which the backing file name is stored.
	BackingFileSize       uint32      //   [16:19] Length of the backing file name in bytes.
	ClusterBits           uint32      //   [20:23] Number of bits that are used for addressing an offset whithin a cluster.
	Size                  uint64      //   [24:31] Virtual disk size in bytes.
	CryptMethod           cryptMethod //   [32:35] Crypt method.
	L1Size                uint32      //   [36:39] Number of entries in the active L1 table.
	L1TableOffset         uint64      //   [40:47] Offset into the image file at which the active L1 table starts.
	RefcountTableOffset   uint64      //   [48:55] Offset into the image file at which the refcount table starts.
	RefcountTableClusters uint32      //   [56:59] Number of clusters that the refcount table occupies.
	NbSnapshots           uint32      //   [60:63] Number of snapshots contained in the image.
	SnapshotsOffset       uint64      //   [64:71] Offset into the image file at which the snapshot table starts.
	IncompatibleFeatures  uint64      //   [72:79] for version >= 3: Bitmask of incomptible feature.
	CompatibleFeatures    uint64      //   [80:87] for version >= 3: Bitmask of compatible feature.
	AutoclearFeatures     uint64      //   [88:95] for version >= 3: Bitmask of auto-clear feature.
	RefcountOrder         uint32      //   [96:99] for version >= 3: Describes the width of a reference count block entry.
	HeaderLength          uint32      // [100:103] for version >= 3: Length of the header structure in bytes.
}
