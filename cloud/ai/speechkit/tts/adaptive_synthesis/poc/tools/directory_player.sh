ext=$1
directory=$2
blob="$directory/*.$ext"

echo "Directory $directory"
# shopt -s nullglob
for fullfile in $blob
do
    filename=$(basename -- "$fullfile")
    filename="${filename%.*}"
    echo "Processing $filename"
    f="$directory/$filename"
    echo $f
    cat $f.txt && afplay $f.$ext
    echo ""
done
