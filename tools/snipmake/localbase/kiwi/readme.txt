Use these scripts to convert kiwi docs into snippet contexts.

0. Prepare the required binaries and data files.
   Required binaries (place in ./bin/):
    - kwworm
    - fakeprewalrus
    - prewalrusfilter
    - tar_to_urldat
    - grattrgen
    - basesearch
    - hamsterwheel
   Data files:
    - (no longer needed?) ./data/config/poly.trie (copy from twalrus or ask a colleague)
    - ./PrewalrusExport.query (copy the PrewalrusToWalrusExport program text from a kiwi cluster or find it in the arcadia tree)
   URL list:
    - put into ./urls.txt

1. (optional; only if you don't feel like using the production kiwi) 
   Load documents into the local kiwi instance:
    - run something like this in a separate shell (in a screen/tmux):
      ya kiwi local --working-dir=local_kiwi --port-shift=666 --get-robot-triggers=PRODUCTION
      This will start a local instance with triggers/metadata copied from production.
    - ya build ../kiwi-transfer
    - see copy_docs_between_kiwis.sample.sh

2. Check and adjust parameters in the fetch_urls.sh script header

3. Run fetch_urls.sh

