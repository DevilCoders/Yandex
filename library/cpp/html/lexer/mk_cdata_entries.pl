#!/usr/bin/perl
use strict;

my $input = $ARGV[0] || "../spec/tags.gperf";

sub get_tags() {
    open G,$input or die 'cannot open gperf';

    my $tags=[];
    while(<G>) {
        m/^(\w+).*HT_lit/ && push @$tags,$1;
    }
    close G;
    return $tags;
}

sub gen_c($) {
    my $tags = shift or die;
    my $ret = "";
    for my $tag(@$tags) {
        $ret .=
"                case HT_$tag: fhold; fgoto cdata_\L$tag;\n";
    }
    return $ret;
}

sub gen_rl($) {
    my $tags = shift or die;

    my $ret = '
%%{
    machine HTLexer;
';

    my $template='
    cdata_$tag := ( any* :>> (
            "</" >a_cdata_text /$tag/i >start_name %end_name gi_end @a_cdata_close
        ) )+ >a_start >a_in_cdata $/a_text;
';

    for my $tag(@$tags) {
        $tag = "\L$tag";
        my $str=$template;
        $str=~s/\$tag/$tag/g or die;
        $ret .= $str;
    }

    $ret .= '
}%%
';

    return $ret;
}

sub main() {
    my $mode = 'rl';

    if(grep /^-c$/,@ARGV) { $mode = 'c'; }
    my $tags = get_tags;

    if($mode eq 'c') {
        print gen_c($tags);
    } else {
        print gen_rl($tags);
    }
}
main;
