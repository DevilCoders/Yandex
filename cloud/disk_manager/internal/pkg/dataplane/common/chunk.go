package common

////////////////////////////////////////////////////////////////////////////////

type Chunk struct {
	ID         string
	Index      uint32
	Data       []byte
	Zero       bool
	StoredInS3 bool
}
