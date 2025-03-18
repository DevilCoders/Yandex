const gulp = require('gulp');
const uglify = require('gulp-uglify');
const obfuscator = require('gulp-javascript-obfuscator');
const wrap = require('gulp-wrap-js');
const rename = require('gulp-rename');
const replace = require('gulp-replace');

const packageJson = require('./package.json');

gulp.task('default', function() {
    return gulp.src('src/loader*.js')
        .pipe(replace('__VERSION__', `"${packageJson.version}"`))
        .pipe(gulp.dest('dist'))
        .pipe(obfuscator({
            stringArrayThreshold: 1, // кодировать все строки
            stringArrayEncoding: ['base64'],
            rotateUnicodeArray: true,
            transformObjectKeys: true,
        }))
        // Убираем использование глобальной atob
        // js-obfuscator использует ее для кодирования строк
        // настроить это никак нельзя
        // иной выход - fork js-obfuscator и gulp-javascript-obfuscator
        .pipe(replace(/\(function\s?\(\)\s?{.*?}\(\)\);/gi, ''))
        .pipe(replace('atob', '(function(b){b=String(b).replace(/=+$/,"");for(var c=0,d,a,f=0,e="";a=b.charAt(f++);~a&&(d=c%4?64*d+a:a,c++%4)?e+=String.fromCharCode(255&d>>(-2*c&6)):0)a="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=".indexOf(a);return e})'))
        // Оборачиваем в функцию для изоляции кода
        .pipe(wrap('(function(){%= body %})()'))
        .pipe(uglify())
        // Пишем свою функцию charCodeAt, тк она ломается Ublock'ом
        .pipe(replace(/\b(\w)\.charCodeAt\((\w+)\)/gi, '(function(a,b){return" !\\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~".indexOf(a[b])+32})($1,$2)'))
        .pipe(rename(path => {
            path.extname = '.min.js'
        }))
        .pipe(gulp.dest('dist'));
});
