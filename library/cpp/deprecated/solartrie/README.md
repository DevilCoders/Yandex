Use library/cpp/on_disk/codec_trie if you need a trie with compression for both keys & values. It's much faster while offering similar compression rate.
Use library/cpp/on_disk/coded_blob if you need compression for values only.
