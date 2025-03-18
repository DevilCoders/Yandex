while : ; do
    seed=$RANDOM$RANDOM
    sed -i.back -E 's/seed: [0-9]+/seed: '"$seed"'/g' ./gulpfile.js
    NODE_ENV=loader node ./node_modules/gulp/bin/gulp.js
    fileSize=$( wc -c < dist/loader.min.js )
    echo $fileSize $seed
    [[ "$fileSize" -gt "4500" ]] || break
done