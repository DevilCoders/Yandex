package generator

import (
	"crypto/rand"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"strconv"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// TODO: define dedicated types for different identifiers [https://st.yandex-team.ru/MDB-5724]

type IDGenerator interface {
	Generate() (string, error)
}

// ClusterIDGenerator generates cluster ID according to cloud rules
type ClusterIDGenerator struct {
	cloudIDPrefix string
}

func (g *ClusterIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

func NewClusterIDGenerator(prefix string) IDGenerator {
	return &ClusterIDGenerator{prefix}
}

var _ IDGenerator = &ClusterIDGenerator{defaultCloudIDPrefix}

// SubClusterIDGenerator generates sub-cluster ID according to cloud rules
type SubClusterIDGenerator struct {
	cloudIDPrefix string
}

func (g *SubClusterIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

func NewSubClusterIDGenerator(prefix string) IDGenerator {
	return &SubClusterIDGenerator{prefix}
}

var _ IDGenerator = &SubClusterIDGenerator{defaultCloudIDPrefix}

// ShardIDGenerator generates shard ID according to cloud rules
type ShardIDGenerator struct {
	cloudIDPrefix string
}

func (g *ShardIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

func NewShardIDGenerator(prefix string) IDGenerator {
	return &ShardIDGenerator{prefix}
}

var _ IDGenerator = &ShardIDGenerator{defaultCloudIDPrefix}

// PillarIDGenerator generates target pillar ID according to cloud rules
type PillarIDGenerator struct {
	cloudIDPrefix string
}

func (g *PillarIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

func NewPillarIDGenerator(prefix string) IDGenerator {
	return &PillarIDGenerator{prefix}
}

var _ IDGenerator = &PillarIDGenerator{defaultCloudIDPrefix}

// TaskIDGenerator generates task ID according to cloud rules
type TaskIDGenerator struct {
	cloudIDPrefix string
}

func (g *TaskIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

func NewTaskIDGenerator(prefix string) IDGenerator {
	return &TaskIDGenerator{prefix}
}

var _ IDGenerator = &TaskIDGenerator{defaultCloudIDPrefix}

// EventIDGenerator generates event ID according to cloud rules
type EventIDGenerator struct {
	cloudIDPrefix string
}

func (g *EventIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

type fileSequenceGenerator struct {
	// Path where the cache should be located
	Path string
}

func (g *fileSequenceGenerator) Generate(fileName string) int64 {
	p := path.Join(g.Path, fileName)
	var cur int64
	read, err := ioutil.ReadFile(p)
	if err != nil {
		if err = os.MkdirAll(path.Dir(p), 0777); err != nil {
			panic(fmt.Sprintf("failed to create directories at %q: %s", path.Dir(p), err))
		}
	} else {
		if cur, err = strconv.ParseInt(string(read), 10, 64); err != nil {
			panic(fmt.Sprintf("failed to parse current sequence id %q from file %q: %s", string(read), p, err))
		}
	}

	cur++

	if err = ioutil.WriteFile(p, []byte(strconv.FormatInt(cur, 10)), 0666); err != nil {
		panic(fmt.Sprintf("failed to write sequence id to file %q: %s", p, err))
	}

	return cur
}

// FileSequenceIDGenerator caches IDs on filesystem
type FileSequenceIDGenerator struct {
	// prefix for id, also used as filename to store cache
	prefix string
	// Path where the cache should be located
	sequenceGenerator fileSequenceGenerator
}

func NewFileSequenceIDGenerator(prefix, path string) *FileSequenceIDGenerator {
	return &FileSequenceIDGenerator{
		prefix:            prefix,
		sequenceGenerator: fileSequenceGenerator{Path: path},
	}
}

var _ IDGenerator = &FileSequenceIDGenerator{}

func (g *FileSequenceIDGenerator) Generate() (string, error) {
	cur := g.sequenceGenerator.Generate(g.prefix)
	return fmt.Sprintf("%s%d", g.prefix, cur), nil
}

const (
	cloudIDGenPartLen    = 17
	cloudValidIDRunes    = "abcdefghijklmnopqrstuv0123456789"
	defaultCloudIDPrefix = "mdb"

	hostnameUbuntuLen  = uint64(16)
	HostnameWindowsLen = uint64(10)
	hostnameValidRunes = "abcdefghijklmnopqrstuv0123456789"
)

func generateRandomN(n int) ([]byte, error) {
	choice := make([]byte, n)
	return choice, generateRandom(choice)
}

func generateRandom(choice []byte) error {
	if n, err := rand.Read(choice); err != nil {
		return xerrors.Errorf("random generated only for %d bytes instead of %d: %w", n, len(choice), err)
	}

	return nil
}

// generateID generates ID according to cloud rules
func generateID(prefix string) (string, error) {
	suffix := make([]byte, cloudIDGenPartLen)
	if err := generateRandom(suffix); err != nil {
		return "", err
	}

	for i := range suffix {
		suffix[i] = cloudValidIDRunes[int(suffix[i])%len(cloudValidIDRunes)]
	}

	if prefix == "" {
		prefix = defaultCloudIDPrefix
	}

	return prefix + string(suffix), nil
}

type PlatformHostnameGenerator interface {
	Generate(prefix, suffix string, platform compute.Platform) (string, error)
}

type platformHostnameGeneratorImpl struct {
	generators map[compute.Platform]HostnameGenerator
}

func NewPlatformHostnameGenerator() PlatformHostnameGenerator {
	return &platformHostnameGeneratorImpl{generators: map[compute.Platform]HostnameGenerator{
		compute.Ubuntu:  NewRandomHostnameUbuntuGenerator(),
		compute.Windows: NewRandomHostnameWindowsGenerator(),
	}}
}

func NewPlatformTestHostnameGenerator(m map[compute.Platform]HostnameGenerator) PlatformHostnameGenerator {
	return &platformHostnameGeneratorImpl{generators: m}
}

func (g *platformHostnameGeneratorImpl) Generate(prefix, suffix string, platform compute.Platform) (string, error) {
	if generator, ok := g.generators[platform]; ok {
		return generator.Generate(prefix, suffix)
	}
	return "", xerrors.Errorf("no generator for platform id %d", platform)
}

type HostnameGenerator interface {
	Generate(prefix, suffix string) (string, error)
}

type RandomHostnameGenerator struct {
	length uint64
}

func NewRandomHostnameUbuntuGenerator() *RandomHostnameGenerator {
	return &RandomHostnameGenerator{length: hostnameUbuntuLen}
}

func NewRandomHostnameWindowsGenerator() *RandomHostnameGenerator {
	return &RandomHostnameGenerator{length: HostnameWindowsLen}
}

var _ HostnameGenerator = &RandomHostnameGenerator{}

func (r *RandomHostnameGenerator) Generate(prefix, suffix string) (string, error) {
	random, err := generateHostname(r.length)
	if err != nil {
		return "", err
	}

	return fmt.Sprintf("%s%s%s", prefix, random, suffix), nil
}

func generateHostname(length uint64) (string, error) {
	hostname := make([]byte, length)
	if err := generateRandom(hostname); err != nil {
		return "", err
	}

	for i := range hostname {
		hostname[i] = hostnameValidRunes[int(hostname[i])%len(hostnameValidRunes)]
	}

	return string(hostname), nil
}

type FileSequenceHostnameGenerator struct {
	// Used as filename prefix to store cache
	prefix            string
	sequenceGenerator fileSequenceGenerator
}

func NewFileSequenceHostnameGenerator(prefix, path string) *FileSequenceHostnameGenerator {
	return &FileSequenceHostnameGenerator{
		prefix:            prefix,
		sequenceGenerator: fileSequenceGenerator{Path: path},
	}
}

var _ HostnameGenerator = &FileSequenceHostnameGenerator{}

func (g *FileSequenceHostnameGenerator) Generate(prefix, suffix string) (string, error) {
	fileName := fmt.Sprintf("%s-%s", g.prefix, prefix)
	cur := g.sequenceGenerator.Generate(fileName)
	return fmt.Sprintf("%s%d%s", prefix, cur, suffix), nil
}

// BackupIDGenerator generates backup ID according to cloud rules
type BackupIDGenerator struct {
	cloudIDPrefix string
}

func (g *BackupIDGenerator) Generate() (string, error) {
	return generateID(g.cloudIDPrefix)
}

var _ IDGenerator = &BackupIDGenerator{defaultCloudIDPrefix}

// RandomIDGenerator generates prefixed ID according to cloud rules
type RandomIDGenerator struct{}

func (g *RandomIDGenerator) Generate() (string, error) {
	return generateID("")
}

func NewRandomIDGenerator() IDGenerator {
	return &RandomIDGenerator{}
}

var _ IDGenerator = &RandomIDGenerator{}

type UUIDGenerator struct {
	g uuid.Generator
}

func NewUUIDGenerator() IDGenerator {
	return &UUIDGenerator{g: uuid.NewGen()}
}

func (ug *UUIDGenerator) Generate() (string, error) {
	id, err := ug.g.NewV4()
	return id.String(), err
}

var _ IDGenerator = &UUIDGenerator{}

type SequentialGenerator struct {
	i int64
}

func NewSequentialGenerator(start int64) IDGenerator {
	return &SequentialGenerator{
		i: start,
	}
}

func (s *SequentialGenerator) Generate() (string, error) {
	defer func() { s.i++ }()
	return strconv.FormatInt(s.i, 10), nil
}

var _ IDGenerator = &SequentialGenerator{}
