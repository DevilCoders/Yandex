Source: liblwp-protocol-http-socketunixalt-perl
Section: perl
Priority: optional
Build-Depends: debhelper (>= 7)
Build-Depends-Indep: perl (>= 5.10.1), liblibwww-perl-perl
Maintainer: Evgeniy Zaitsev <eightn@yandex-team.ru>
Standards-Version: 3.8.3
Homepage: http://search.cpan.org/dist/LWP-Protocol-http-SocketUnixAlt/

Package: liblwp-protocol-http-socketunixalt-perl
Architecture: all
Depends: ${perl:Depends}, ${misc:Depends}, perl (>= 5.10.1),
  liblibwww-perl-perl
Description: Speak HTTP through Unix sockets
 LWP::Protocol::http::UnixSocketAlt is a fork of Florian Ragwitz's
 LWP::Protocol::http::SocketUnix 0.02. It fixes a few issues including:
 .
 This description was automagically extracted from the module by dh-make-perl.
