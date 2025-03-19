package mapitem

// http://manpages.ubuntu.com/manpages/trusty/man1/qemu-img.1.html (map [-f fmt] [--output=ofmt] filename)
type ImageMapItem struct {
	Start            int64   `json:"start"`             // Offset in logical disk inside image.
	Length           int64   `json:"length"`            // Length block in bytes.
	Depth            int64   `json:"depth"`             // Depth of images (file which contain raw block).
	Zero             bool    `json:"zero"`              // Block contain only zeroes.
	Data             bool    `json:"data"`              // Block contain data.
	RawImageOffset   *int64  `json:"offset,omitempty"`  // Offset in file for direct read raw data.
	CompressedOffset *uint64 `json:"coffset,omitempty"` // Offset in file for compressed data.
	CompressedSize   *uint64 `json:"csize,omitempty"`   // Size of compressed block.
}

func (i ImageMapItem) HasNonZeroContent() bool {
	return i.Data && !i.Zero && i.RawImageOffset != nil
}

func (i ImageMapItem) SameContentTypeTo(b ImageMapItem) bool {
	return i.Data == b.Data && i.Zero == b.Zero
}

func (i ImageMapItem) IsPrevTo(b ImageMapItem) bool {
	return b.Start == i.Start+i.Length
}

func (i ImageMapItem) RawIsPrevTo(b ImageMapItem) bool {
	return i.HasNonZeroContent() == b.HasNonZeroContent() &&
		(!i.HasNonZeroContent() || *b.RawImageOffset == *i.RawImageOffset+i.Length)
}
