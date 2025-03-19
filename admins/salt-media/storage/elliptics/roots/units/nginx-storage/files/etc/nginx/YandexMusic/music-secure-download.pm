{%- set secrets = pillar['music-secrets'] %}
# Lua implementation of secure download protocol described here:
# http://wiki.yandex-team.ru/muz/dev/DownloadProtocol
#
# Ported from lua script (see ../lua) by Ilya Roubin.

package YandexMusic;

use nginx;
use strict;
use warnings;

use Digest::MD5 'md5_hex';
use Digest::SHA qw(hmac_sha512_hex hmac_sha256_hex);

# time in seconds after which token will expire
my $tokenExpirationSeconds = 60;
# set to 1 for quick no-expire debug mode
my $ignoreInvalidTimestamps = 0;

# secret server key shared with the proxy (token generation)
my $Ks_default  = "{{ secrets['__default'] }}";
my $Ks_noexpire = "{{ secrets['__noexpire'] }}";

# secret server key shared with the proxy (music_uid cookie validation)
my $Kc = "{{ secrets['__kc'] }}";

my $BAD_TOKEN = 0;
my $OK_TOKEN_FOR_DEFAULT = 1;
my $OK_TOKEN_FOR_NOEXPIRE = 2;

sub ping() {
    return OK;
}

sub checkToken() {
    my $r = shift;

    sub nginx_log($$) {
        my ($r, $msg) = @_;
        $r->log_error(0, $msg);
    }

    # validation methods

    sub isValidTokenForKey($$$$$$$$$$) {
        my ($r, $token, $timestamp, $filePath, $host, $cookie, $from, $userAgent, $Ks, $blobSignFromUrl) = @_;

        # https://wiki.yandex-team.ru/muz/dev/external-api/spec#user-agentix-yandex-music-client
        # https://wiki.yandex-team.ru/muz/dev/external-api/spec/json-spec#x-yandex-music-clientiuser-agent
        my $clientKey;

        if (1 > 2) {
        }
        {%- for prefix in secrets %}
        {%- if not prefix.startswith('__') %}
        elsif ($userAgent =~ m:^{{ prefix }}/:) {
            $clientKey = '{{ secrets[prefix] }}';
        }
        {%- endif %}
        {%- endfor %}
        elsif ($userAgent =~ m:^Elements:) {
            $clientKey = '{{ secrets["Elements"] }}';
        }
        elsif ($from eq "mobile") {   # XXX deprecated, delete me
            $clientKey = '{{ secrets["YandexMusic"] }}';
        }
        else {
            $clientKey = '{{ secrets["__web"] }}';
        }

        # to be deprecated soon
        my $old_signInput = $Ks . $timestamp . $cookie;
        my $old_sign = md5_hex($old_signInput);
        my $old_validTokenInput = $clientKey . $filePath . $old_sign;
        my $old_validToken = md5_hex($old_validTokenInput);

        # https://jira.yandex-team.ru/browse/MUZ-4311
        # https://jira.yandex-team.ru/browse/MUZ-4209
        my $old_blob_signInput = $Ks . $timestamp . $cookie . "/" . $filePath;
        my $old_blob_sign = md5_hex($old_blob_signInput);
        my $old_blob_validTokenInput = $clientKey . $filePath . $old_blob_sign;
        my $old_blob_validToken = md5_hex($old_blob_validTokenInput);

        # https://jira.yandex-team.ru/browse/MUZ-7558
        my $blob_signInput = "scheme://" . $host . "/" . $filePath . "?time=" . $timestamp;
        my $blob_sign = hmac_sha512_hex($blob_signInput, $Ks);
        my $blob_validTokenInput = $clientKey . $filePath . "/" . $blobSignFromUrl . $blobSignFromUrl;
        my $blob_validToken = md5_hex($blob_validTokenInput);

        my $nblob_signInput = $host . "/" . $filePath . "/" . $timestamp;
        my $nblob_sign = hmac_sha256_hex($nblob_signInput, $Ks);
        my $nblob_validTokenInput = $clientKey . $filePath . $nblob_sign;
        my $nblob_validToken = md5_hex($nblob_validTokenInput);

        my $result = ($token eq $old_validToken or $token eq $old_blob_validToken or $token eq $blob_validToken or $token eq $nblob_validToken);
        #my $result = ($token eq $old_validToken or $token eq $old_blob_validToken or $token eq $blob_validToken);

        unless ($result) {
            nginx_log($r, "Failed to verify token. (with Ks=$Ks)");
            nginx_log($r, ":::: request timestamp=$timestamp filePath=$filePath token=$token from=$from");
            nginx_log($r, ":::: old sign = md5($old_signInput) = $old_sign");
            nginx_log($r, ":::: old token = md5($old_validTokenInput) = $old_validToken");
            nginx_log($r, ":::: old blob sign = md5($old_blob_signInput) = $old_blob_sign");
            nginx_log($r, ":::: old blob token = md5($old_blob_validTokenInput) = $old_blob_validToken");
            nginx_log($r, ":::: blob sign = hmac_sha512_hex($blob_signInput, $Ks) = $blob_sign");
            nginx_log($r, ":::: blob sign from url = $blobSignFromUrl");
            nginx_log($r, ":::: blob token = md5($blob_validTokenInput) = $blob_validToken");
            nginx_log($r, ":::: new blob sign = hmac_sha256_hex($nblob_signInput, $Ks) = $nblob_sign");
            nginx_log($r, ":::: new blob token = md5($nblob_validTokenInput) = $nblob_validToken");
            nginx_log($r, ":::: userAgent = $userAgent");
        }

        return $result;
    }

    sub isValidToken($$$$$$$$$) {
        my ($r, $token, $timestamp, $filePath, $host, $cookie, $from, $userAgent, $blobSignFromUrl) = @_;
        if (isValidTokenForKey($r, $token, $timestamp, $filePath, $host, $cookie, $from, $userAgent, $Ks_default, $blobSignFromUrl)) {
            return $OK_TOKEN_FOR_DEFAULT;
        } elsif (isValidTokenForKey($r, $token, $timestamp, $filePath, $host, $cookie, $from, $userAgent, $Ks_noexpire, $blobSignFromUrl)) {
            return $OK_TOKEN_FOR_NOEXPIRE;
        } else {
            return $BAD_TOKEN;
        }
    }

    sub isValidTimestamp($$) {
        my ($r, $ts) = @_;

        my $curTime = time();
        my $timeFromToken = $ts;

        # if $ts is not in seconds
        while ($timeFromToken / $curTime > 2) {
            $timeFromToken = $timeFromToken / 10;
        }

        my $tokenAgeSeconds = $curTime - $timeFromToken;
        my $result = $tokenAgeSeconds <= $tokenExpirationSeconds;

        unless ($result) {
            nginx_log($r, "Failed to verify timestamp.");
            nginx_log($r, ":::: current time: " . sprintf("%08x", $curTime));
            nginx_log($r, ":::: timestamp from token: " . sprintf("%08x", $timeFromToken));
            nginx_log($r, ":::: token age in seconds: $tokenAgeSeconds");
            nginx_log($r, ":::: token expiration period in seconds: $tokenExpirationSeconds");
        }

        return $result;
    }

    # remove this sub - don't use cookie anymore
    sub isValidCookie($$) {
        my ($r, $cookie) = @_;

        if ($cookie eq "") {
            return 1;
        }

        my ($rnd, $timestamp, $md5) = $cookie =~ /([a-z0-9]+)\.([a-z0-9]+)\.([a-z0-9]+)/;
        unless ($rnd and $timestamp and $md5) {
            nginx_log($r, "cookie format is wrong: " . $cookie);
            return 0;
        }

        my $cookie_md5_input = $Kc . $rnd . $timestamp;
        my $validMd5 = md5_hex($cookie_md5_input);
        my $result = ($md5 eq $validMd5);

        unless ($result) {
            nginx_log($r, "Failed to verify cookie: $cookie");
            nginx_log($r, ":::: rnd: " . $rnd);
            nginx_log($r, ":::: timestamp: " . $timestamp);
            nginx_log($r, ":::: md5 input for cookie: " . $cookie_md5_input);
            nginx_log($r, ":::: generated md5: " . $validMd5);
            nginx_log($r, ":::: md5 from cookie: " . $md5);
        }
        return $result;
    }

    # helper error diagnostics methods

    sub error($$$) {
        my ($r, $code, $text) = @_;
        nginx_log($r, "FAIL with code $code: $text");
        return $code;
    }

    sub invalidParams($$) {
        my ($r, $msg) = @_;
        return error($r, 422, "invalid params: $msg");
    }

    sub forbidden($$) {
        my ($r, $msg) = @_;
        return error($r, 403, "forbidden: $msg");
    }

    sub fileHasGone($$) {
        my ($r, $msg) = @_;
        return error($r, 410, "token has expired for the file: $msg");
    }

    # handler start
    my $host = $r->header_in("Host");
    my $uri = $r->uri;
    my $cookies = $r->header_in("Cookie");
    my $range = $r->header_in("Range");
    my $userAgent = $r->header_in("X-Yandex-Music-Client");

    nginx_log($r, "===== Starting handling of request, uri=$uri");

    unless ($host) {
        return invalidParams($r, "invalid host");
    }

    unless ($cookies) {
        $cookies = "";
    }

    unless ($range) {
        $range = "";
    }

    unless ($userAgent) {
        $userAgent = "";
    }

    if ($userAgent eq "") {  # fallback
        $userAgent = $r->header_in("User-Agent");
    }

    my ($token, $timestamp, $filePath, $blobSignFromUrl) = ($uri =~ m,/get-mp3/([a-z0-9]+)/([a-z0-9]+)/(rmusic/[a-zA-Z0-9_-]+)/([a-zA-Z0-9_-]+)/?$,);
    unless ($token and $timestamp and $filePath and $blobSignFromUrl) {
        return invalidParams($r, "failed to parse uri: $uri");
    }

    nginx_log($r, "token=$token ts=$timestamp fp=$filePath");

    unless ($timestamp =~ /[0-9a-z]+/) {
        return invalidParams($r, "failed to parse timestamp: $timestamp");
    }
    my $timestampN = 0 + hex $timestamp;
    nginx_log($r, "timestampN: $timestampN");
    # check later

    nginx_log($r, "cookies: $cookies");
    $cookies =~ /music_uid=([0-9a-z.]+)/;
    my $cookie = $1;
    unless ($cookie) {
        $cookie = "";
    }

    unless (isValidCookie($r, $cookie)) {
        return forbidden($r, "music_uid is not valid");
    }

    $r->args =~ /from=([a-z0-9A-Z_\-]+)/;
    my $from = $1;
    unless ($from) {
        $from = "";
    }

    my $tokenType = isValidToken($r, $token, $timestamp, $filePath, $host, $cookie, $from, $userAgent, $blobSignFromUrl);
    if ($tokenType == $BAD_TOKEN) {
        # if cookie is set, but token was calculated without it
        $tokenType = isValidToken($r, $token, $timestamp, $filePath, $host, "", $from, $userAgent, $blobSignFromUrl);
        if ($tokenType == $BAD_TOKEN) {
            return forbidden($r, "token is not valid");
        }
    }

    # https://st.yandex-team.ru/MUSIC-1638
    my $notFirstRangedRequest = 0;
    if ($range =~ /bytes=.*/) {
        nginx_log($r, "range: $range");
        unless ($range =~ /bytes=0?\-.*/) {
            $notFirstRangedRequest = 1;
        }
    }

    unless ($ignoreInvalidTimestamps or $tokenType == $OK_TOKEN_FOR_NOEXPIRE or $notFirstRangedRequest or isValidTimestamp($r, $timestampN)) {
        return fileHasGone($r, $filePath);
    }

    return OK;
}

1;
