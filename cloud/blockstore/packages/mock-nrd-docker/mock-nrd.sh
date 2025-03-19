for i in {0..3}
do
  touch /Berkanavt/nbs-server/data/nonrepl-$i.bin
  truncate -s 1024M /Berkanavt/nbs-server/data/nonrepl-$i.bin
done
