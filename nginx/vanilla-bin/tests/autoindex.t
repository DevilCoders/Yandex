#!/usr/bin/perl

# (C) Maxim Dounin

# Tests for autoindex module.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http autoindex charset symlink/)->plan(16)
	->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location / {
            autoindex on;
        }
        location /utf8/ {
            autoindex on;
            charset utf-8;
        }
    }
}

EOF

my $d = $t->testdir();

mkdir("$d/test-dir");
symlink("$d/test-dir", "$d/test-dir-link");

$t->write_file('test-file', '');
symlink("$d/test-file", "$d/test-file-link");

$t->write_file('test-colon:blah', '');
$t->write_file('test-long-' . ('0' x 120), '');         # 120: length('test-long-' . ('0' x 120)) > NGX_HTTP_AUTOINDEX_NAME_LEN (120)
$t->write_file('test-long-' . ('>' x 120), '');         # 120: ... the same
$t->write_file('test-escape-url-%', '');
$t->write_file('test-escape-url2-?', '');
$t->write_file('test-escape-html-<>&', '');

mkdir($d . '/utf8');
$t->write_file('utf8/test-utf8-' . ("\xd1\x84" x 3), '');
$t->write_file('utf8/test-utf8-' . ("\xd1\x84" x 120), '');                     # 120: ... the same
$t->write_file('utf8/test-utf8-<>&-' . "\xd1\x84", '');
$t->write_file('utf8/test-utf8-<>&-' . ("\xd1\x84" x 120), '');                 # 120: ... the same
$t->write_file('utf8/test-utf8-' . ("\xd1\x84" x 3) . '-' . ('>' x 120), '');   # 120: ... the same

mkdir($d . '/test-dir-escape-<>&');

$t->run();

###############################################################################

my $r = http_get('/');

like($r, qr!href="test-file"!ms, 'file');
like($r, qr!href="test-file-link"!ms, 'symlink to file');
like($r, qr!href="test-dir/"!ms, 'directory');
like($r, qr!href="test-dir-link/"!ms, 'symlink to directory');

unlike($r, qr!href="test-colon:blah"!ms, 'colon not scheme');
like($r, qr!test-long-0{107}\.\.&gt;!ms, 'long name');                          # 107 == 120 - length('test-long-') - length('..>')

like($r, qr!href="test-escape-url-%25"!ms, 'escaped url');
like($r, qr!href="test-escape-url2-%3f"!msi, 'escaped ? in url');
like($r, qr!test-escape-html-&lt;&gt;&amp;!ms, 'escaped html');
like($r, qr!test-long-(&gt;){107}\.\.&gt;!ms, 'long escaped html');             # 107 == 120 - length('test-long-') - length('..>')

$r = http_get('/utf8/');

like($r, qr!test-utf8-(\xd1\x84){3}</a>!ms, 'utf8');
like($r, qr!test-utf8-(\xd1\x84){107}\.\.!ms, 'utf8 long');                     # 107 == 120 - length('test-utf8-') - length('..>')

like($r, qr!test-utf8-&lt;&gt;&amp;-\xd1\x84</a>!ms, 'utf8 escaped');
like($r, qr!test-utf8-&lt;&gt;&amp;-(\xd1\x84){103}\.\.!ms,                     # 103 == 120 - length('test-utf8-<>&-') - length('..>')
	'utf8 escaped long');
like($r, qr!test-utf8-(\xd1\x84){3}-(&gt;){103}\.\.!ms, 'utf8 long escaped');   # 103 == 120 - length('test-utf8-???-') - length('..>')

like(http_get('/test-dir-escape-<>&/'), qr!test-dir-escape-&lt;&gt;&amp;!ms,
	'escaped title');

###############################################################################
