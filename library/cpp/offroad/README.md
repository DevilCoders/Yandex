# Dictionary-based compression library for streams of integers

The `offroad` name was given (by mvel@) after library's internal name, `blk4x4`
(introduced by ironpeter@). Like offroad vehicle that can ride every
road, this codec can compress almost all imaginable sorts of data.

## Directory structure
  * `byte_stream` — implementation of `TInputStream` & `TOutputStream` using offroad codec for compression.
  * `codec` — low-level compression primitives.
  * `custom` — offroad serializer/vectorizer adaptors for some common types.
  * `fat` — implementation of fast access table for key files.
  * `flat` — simple bit codec for integer tuples. Calculates bitness for a given array of tuples, and then writes them out, thus providing O(1) random read access.
  * `index` — reader/writer for offroad-compressed keyinv index. These ones are pretty tough to use directly, try looking into standard ones in `standard` folder.
  * `key` — reader/writer for offroad-compressed key files.
  * `standard` — standard readers/writers for common use cases.
  * `streams` — value and vector bit streams. This is what offroad codecs work on top of.
  * `sub` — subindex reader/writer adaptors. Subindex itself is flat-compressed.
  * `tuple` — this is what offroad is all about, readers/writers for streams of integer tuples.
  * `wad` — implementation of a simple file format that contains blobs accessible by index. Subindex is flat-compressed.
