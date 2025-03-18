#!/usr/bin/env perl

package Jing;

use lib $ENV{'SHUTTER_ROOT'}.'/share/shutter/resources/modules';

use utf8;
use strict;
use POSIX qw/setlocale/;
use Locale::gettext;
use Glib qw/TRUE FALSE/;
use Data::Dumper;
use File::Basename;
use URI::Escape;

use Shutter::Upload::Shared;
our @ISA = qw(Shutter::Upload::Shared);



# change username here to your @yandex-team login (for example, 'elwood')
my $USER_NAME = '__CHANGE_ME_TO_YOUR_LOGIN__';



my $JING_URL = 'https://jing:jing@jingdav.yandex-team.ru';
my $JING_FILES_URL = 'https://jing.yandex-team.ru/files';


my $d = Locale::gettext->domain("shutter-upload-plugins");
$d->dir( $ENV{'SHUTTER_INTL'} );

my %upload_plugin_info = (
    'module' => "Jing",
    'url' => "https://jingdav.yandex-team.ru",
    'description' => "Upload screenshots on server with DAV support",
    'supports_anonymous_upload' => TRUE,
    'supports_authorized_upload' => FALSE,
    'supports_oauth_upload' => FALSE,
);

binmode( STDOUT, ":utf8" );
if ( exists $upload_plugin_info{$ARGV[ 0 ]} ) {
    print $upload_plugin_info{$ARGV[ 0 ]};
    exit;
}

sub new {
    my $class = shift;
    my $self = $class->SUPER::new( shift, shift, shift, shift, shift, shift );
    bless $self, $class;
    return $self;
}

sub init {
    my $self = shift;
    return TRUE;
}

sub upload {
    my ( $self, $upload_filename, $username, $password ) = @_;

    print STDERR "Uploading...\n";
    print STDERR Dumper($upload_filename, $username, $password) . "\n";

    # full url
    my $url = "${JING_URL}/${USER_NAME}/";

    eval {
        my $only_filename = basename($upload_filename);
        $only_filename = uri_escape_utf8($only_filename);

        system('curl', '-gSs', '--upload-file', $upload_filename, $url . $only_filename);

        my $direct_link = "${JING_FILES_URL}/${USER_NAME}/${only_filename}";

        # save all retrieved links to a hash, as an example:
        $self->{_links}->{'direct_link'} = $direct_link;

        # copy current link to clipboard (need xclip program installed)
        system('bash', '-c', 'xclip <(echo -n \'' . $direct_link . '\') -selection c');

        #set success code (200)
        $self->{_links}{'status'} = 200;

    };
    if($@){
        $self->{_links}{'status'} = $@;
    }

    #and return links
    return %{ $self->{_links} };
}

1;
