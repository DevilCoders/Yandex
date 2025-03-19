var gulp        = require("gulp");
var browserify  = require("browserify");
var source      = require("vinyl-source-stream");
var tsify       = require("tsify");
var uglify      = require("gulp-uglify");
var sourceMaps  = require("gulp-sourcemaps");
var buffer      = require("vinyl-buffer");
var ts          = require("gulp-typescript");
var less        = require("gulp-less");
var cleanCSS    = require("gulp-clean-css");
var prefixer    = require("gulp-autoprefixer");
var plumber     = require("gulp-plumber");
var liveReload  = require("gulp-livereload");
var nodemon     = require("gulp-nodemon");


var tsConfig = JSON.parse(require("fs").readFileSync("tsconfig.json"));

gulp.task("markup-js", function () {
    return browserify({
        debug: true,
        entries: ["markup/src/main.tsx"]
    })
    .plugin(tsify, {
        jsx: "react",
    })
    .bundle()
    .on("error", function(err) {
        console.log(err.toString());
        this.emit("end");
    })
    .pipe(source("bundle.js"))
    .pipe(buffer())
    .pipe(sourceMaps.init({loadMaps: true}))
    .pipe(uglify())
    .pipe(sourceMaps.write('.'))
    .pipe(gulp.dest("markup/dst"))
    .pipe(liveReload({ basePath: process.cwd() + '/markup/dst' }))
});

gulp.task("markup-css", function() {
    return gulp.src("markup/src/main.less")
        .pipe(plumber({errorHandler: function(err){console.log(err); this.emit("end");} }))
        .pipe(sourceMaps.init())
        .pipe(less())
        .pipe(prefixer({
            browsers: ['last 2 versions'],
            cascade: false
        }))
        .pipe(cleanCSS())
        .pipe(sourceMaps.write('.'))
        .pipe(gulp.dest("markup/dst"))
        .pipe(liveReload({ basePath: process.cwd() + '/markup/dst' }))
});

var tp = ts.createProject('tsconfig.json');
gulp.task("engine", function(){
    return gulp.src("engine/src/**/*.ts")
        .pipe(tp())
        .pipe(gulp.dest('engine/dst'))
});

gulp.task("nodemon", function(){
    var stream = nodemon({
            script: 'engine/dst/main.js',
            watch: 'engine/dst',
            })
     stream
         .on('restart', function () {
           console.log('restarted!')
         })
         .on('crash', function() {
           console.error('Application has crashed!\n')
            stream.emit('restart', 10)  // restart the server in 10 seconds
         })
});

gulp.task("watch", ["default", "nodemon"], function(){
    liveReload.listen();
    gulp.watch('engine/src/**/*.ts', {verbose: true}, ['engine'])
    gulp.watch('markup/src/**/*.tsx', {verbose: true}, ['markup-js'])
    gulp.watch('markup/src/**/*.less', {verbose: true}, ['markup-css'])
});

gulp.task("default", ["engine", "markup-css", "markup-js"], function(){});
