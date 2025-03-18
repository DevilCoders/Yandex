const gulp = require('gulp');
const uglify = require('gulp-uglify');
const rename = require('gulp-rename');
const replace = require('gulp-replace');

const packageJson = require('./package.json');

const cookieMatching = process.env.COOKIE_MATCHING && process.env.COOKIE_MATCHING === "true";

gulp.task('default', function() {
    return gulp.src('src/mobile.js')
        .pipe(replace('__VERSION__', `"${packageJson.version}"`))
        .pipe(replace('__COOKIE_MATCHING__', `${cookieMatching}`))
        .pipe(uglify())
        .pipe(rename(`mobile${cookieMatching ? '_cm' : ''}.min.js`))
        .pipe(gulp.dest('dist'));
});