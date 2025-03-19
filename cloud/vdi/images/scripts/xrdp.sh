#!/bin/bash

set -eux

# xrdp & xorgxrdp
echo "==> install xrdp"
DEBIAN_FRONTEND=noninteractive apt install xrdp -y

echo "==> add xrdp user to ssl-cert group"
usermod xrdp -a -G ssl-cert

echo "==> install requirements for xrdp & xorgxrdp & pulseaudio-module-xrdp compilation"
sed -i 's/# deb-src/deb-src/' /etc/apt/sources.list
DEBIAN_FRONTEND=noninteractive apt update --fix-missing -y
DEBIAN_FRONTEND=noninteractive apt install -y \
    autoconf \
    autopoint libfdk-aac-dev \
    bison \
    build-essential \
    dpkg-dev \
    flex \
    g++ \
    gcc \
    git \
    intltool \
    libavcodec-dev  \
    libavformat-dev \
    libfdk-aac-dev  \
    libfuse-dev \
    libgbm-dev \
    libjpeg-dev \
    libjson-c-dev \
    libmp3lame-dev \
    libopus-dev \
    libpam0g-dev \
    libcap-dev \
    libpixman-1-dev \
    libpulse0 \
    libpulse-dev \
    libsndfile1-dev \
    libspeex-dev \
    libspeexdsp-dev \
    libssl-dev \
    libtool \
    libx11-dev \
    libxfixes-dev \
    libxml2-dev \
    libxrandr-dev \
    make \
    nasm \
    pkg-config \
    python-libxml2 \
    xserver-xorg-dev \
    xsltproc \
    xutils \
    xutils-dev

PULSEAUDIO_VERSION=$(pulseaudio --version | awk '{print $2}')
PULSEAUDIO_APT_VERSION=$(apt show pulseaudio | grep "Version: " | awk '{print $2}')

echo "==> download pulseaudio sources (dependency)"
BUILD_DIR=$(mktemp -d)
cd "$BUILD_DIR"
apt source pulseaudio=$PULSEAUDIO_APT_VERSION
cd -

echo "==> configure pulseaudio for compilation"
cd "$BUILD_DIR/pulseaudio-$PULSEAUDIO_VERSION"
./configure
cd -

echo "==> download pulseaudio-module-xrdp sources"
git clone https://github.com/neutrinolabs/pulseaudio-module-xrdp.git "$BUILD_DIR/pulseaudio-module-xrdp"
cd "$BUILD_DIR/pulseaudio-module-xrdp"

echo "==> compile and install pulseaudio-module-xrdp"
./bootstrap
./configure PULSE_DIR="$BUILD_DIR/pulseaudio-$PULSEAUDIO_VERSION"
make
make install
cd -

echo "==> remove build directory"
rm -rf "$BUILD_DIR"

echo "==> fix russian keyboard in xrdp"
cp /etc/xrdp/xrdp_keyboard.ini /etc/xrdp/xrdp_keyboard.ini.backup
sed -i 's/rdp_layout_us=us/rdp_layout_us=us,ru/' /etc/xrdp/xrdp_keyboard.ini
sed -i 's/rdp_layout_ru=ru/rdp_layout_ru=us,ru/' /etc/xrdp/xrdp_keyboard.ini

echo "==> replace xrdp config"
mv /etc/xrdp/xrdp.ini /etc/xrdp/xrdp.ini.backup
tee /etc/xrdp/xrdp.ini > /dev/null <<EOF
[Globals]
ini_version=1

fork=true
port=3389
use_vsock=false
tcp_nodelay=true
tcp_keepalive=true
#tcp_send_buffer_bytes=32768
#tcp_recv_buffer_bytes=32768
security_layer=negotiate
crypt_level=high
certificate=
key_file=
ssl_protocols=TLSv1.3

autorun=
allow_channels=true
allow_multimon=true
bitmap_cache=true
bitmap_compression=true
bulk_compression=true
hidelogwindow=true
max_bpp=32
new_cursors=true
use_fastpath=both

;
; colors used by windows in RGB format
;
blue=009cb5
grey=dedede
#black=000000
#dark_grey=808080
#blue=08246b
#dark_blue=08246b
#white=ffffff
#red=ff0000
#green=00ff00
#background=626c72

;
; configure login screen
;

; Login Screen Window Title
#ls_title=My Login Title

; top level window background color in RGB format
ls_top_window_bg_color=009cb5

; width and height of login screen
ls_width=350
ls_height=430

; login screen background color in RGB format
ls_bg_color=dedede

; optional background image filename (bmp format).
#ls_background_image=

; logo
; full path to bmp-file or file in shared folder
ls_logo_filename=
ls_logo_x_pos=55
ls_logo_y_pos=50

; for positioning labels such as username, password etc
ls_label_x_pos=30
ls_label_width=65

; for positioning text and combo boxes next to above labels
ls_input_x_pos=110
ls_input_width=210

; y pos for first label and combo box
ls_input_y_pos=220

; OK button
ls_btn_ok_x_pos=142
ls_btn_ok_y_pos=370
ls_btn_ok_width=85
ls_btn_ok_height=30

; Cancel button
ls_btn_cancel_x_pos=237
ls_btn_cancel_y_pos=370
ls_btn_cancel_width=85
ls_btn_cancel_height=30

[Logging]
LogFile=xrdp.log
LogLevel=CORE
EnableSyslog=true
SyslogLevel=DEBUG

[Channels]
rdpdr=true
rdpsnd=true
drdynvc=true
cliprdr=true
rail=true
xrdpvr=true
tcutils=true

[Xorg]
name=Xorg
lib=libxup.so
username=ask
password=ask
ip=127.0.0.1
port=-1
code=20
EOF

echo "==> replace sesman config"
mv /etc/xrdp/sesman.ini /etc/xrdp/sesman.ini.backup
tee /etc/xrdp/sesman.ini > /dev/null <<EOF
[Globals]
ListenAddress=127.0.0.1
ListenPort=3350
EnableUserWindowManager=true
UserWindowManager=startwm.sh
DefaultWindowManager=startwm.sh
ReconnectScript=reconnectwm.sh

[Security]
AllowRootLogin=false
MaxLoginRetry=5
TerminalServerUsers=tsusers
TerminalServerAdmins=tsadmins
AlwaysGroupCheck=false
RestrictOutboundClipboard=false

[Sessions]
X11DisplayOffset=10
MaxSessions=1
KillDisconnected=false
DisconnectedTimeLimit=0
IdleTimeLimit=0
Policy=Default

[Logging]
LogFile=xrdp-sesman.log
LogLevel=CORE
EnableSyslog=1
SyslogLevel=DEBUG

[Xorg]
param=/usr/lib/xorg/Xorg
param=-config
param=xrdp/xorg.conf
param=-noreset
param=-nolisten
param=tcp
param=-logfile
param=.xorgxrdp.%s.log

[Chansrv]
FuseMountName=thinclient_drives
FileUmask=077

[SessionVariables]
PULSE_SCRIPT=/etc/xrdp/pulse/default.pa
EOF

echo "==> disable xrdp and xrdp-sesman service"
systemctl disable xrdp.service
systemctl disable xrdp-sesman.service

echo "==> allow 3389/tcp in firewall"
ufw allow 3389/tcp
