#!/usr/bin/env bash
./usage write --scheme-path scheme.js --index-path index.db
./usage read --index-path index.db > out
./usage write_adaptive --scheme-path adaptive_scheme.js --index-path index_adaptive.db
./usage read --index-path index_adaptive.db >> out
./usage write_docurl --src-path docurl.dump --scheme-path index.docurl_scheme.js --index-path index.docurl.db
./usage read --index-path index.docurl.db >> out
