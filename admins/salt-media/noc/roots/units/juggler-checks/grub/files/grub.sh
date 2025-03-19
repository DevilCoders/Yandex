#!/bin/bash
# Inspired by https://github.com/arvidjaar/bootinfoscript

for i; do
   case $i in
      -d|--debug)  _debug=1  ;;
      -i|--info)   _info=1   ;;
      -h|--help)   _help=1   ;;
      -r|--repair) _repair=1 ;;
   esac
done

if [ -z "$_debug" ];then
   exec 3>&2; exec 2>/dev/null
fi


if [ "$_help" ]; then
   echo "Usage $0 -h | [-d] [-i] [-r]
   -h --help      Show this message
   -d --debug     Debug mode
   -i --info      Show info about boot loaders
   -r --repair    Silently try repair bad boot loader and exit
   "
   exit 0
fi

## Create temporary directory ##
tdir=$(mktemp -t -d grub-check-XXXXXXXX);
cd ${tdir}
## Create temporary filenames. ##
tfile=${tdir}/tmp_file
# File to temporarily store an embedded core.img of grub2.
core_img_file=${tdir}/core_img
# File to temporarily store the uncompressed part of core.img of grub2.
core_img_file_unlzma=${tdir}/core_img_unlzma
# File to temporarily store the core.img module of type 2
core_img_file_type_2=${tdir}/core_img_type_2
# Variables
declare -a SYSDisks HDSize HDMBR HDUUID
declare -A HDBL BAD_HDBL

root_dev_md_uuid=""
# Boot Loader string
BL=""
# Error Level
ERROR_Level=$ok

debug(){
   if [ "$_debug" ];then
      lineno=`caller 0`
      echo "`printf "[33mline #%-4s %-12s[0m" ${lineno% *}`: $@"
   fi
}
info(){
   if [ "$_info" ];then
      echo -e "$@"
   fi
}

cleanup () {
  # Clean up
  debug "Cleanup $tdir"
  rm -r $tdir
}

trap cleanup EXIT


repair(){
   if [ -z "$_repair" ]; then
      debug "Not in Repair mode, skip"
      return
   fi
   debug "Repair mode"
   if [ -n "${BAD_HDBL[*]}" ] ; then
      debug "Try repair Bad boot loaders"
      if grub-install "$root_dev" 1>&2; then
         debug "Successfully install grub on $root_dev"
      else
         debug "Installation on $root_dev fail, will install for each disk"
         for hdd in ${!BAD_HDBL[@]}; do
            debug "Try repair Bad boot loader on $hdd"
            grub-install "/dev/$hdd" 1>&2
         done
      fi
   else
      debug "Nothing repaired, congratulations!"
   fi
   exit 0
}

monrun () {
   lineno=`caller 0`
   local level=$1; shift; msg="$@"
   local lcp="`printf "line #%-4s %-8s" ${lineno% *}`"

   echo "PASSIVE-CHECK:grub;${level:-2};${msg:-${lcp}:Something went wrong!}"
   exit $level
}; ok=0; warn=1; crit=2;

grub_modules_loaded () {

    # Check if modules from grub.cfg are installed on system

    local GRUB_CFG="/boot/grub/grub.cfg"
    local INSTALLED_MODS="/boot/grub/i386-pc/"

    # for grub v1
    if [[ ! -d $INSTALLED_MODS ]]
    then
        INSTALLED_MODS="/boot/grub"
    fi

    declare -a MODS_IN_CFG="($(awk '/insmod/ { print $2 }' $GRUB_CFG | sort | uniq))"
    declare -a MODS_ON_SYS="($(ls $INSTALLED_MODS | awk -F "." '/mod$/ { print $1 }'))"

    for MOD in ${MODS_IN_CFG[@]}; do
        if [[ " ${MODS_ON_SYS[@]} " =~ " $MOD " ]]
        then
           # echo $MOD
            continue
        elif [[ " ${MODS_ON_SYS[@]} " =~ " all_video "
               && "efi_gop efi_uga ieee1275_fb" =~ "$MOD" ]]
        then
            continue
        else
            monrun $crit "Module not installed: " $MOD
        fi
    done
}

grub_modules_loaded

# check virtual host via yandex-lib-autodetect-environment package
if [ -f /usr/local/sbin/autodetect_environment ] ; then
   . /usr/local/sbin/autodetect_environment
   if [ $is_virtual_host -eq 1 ]; then
      if [ -z "$_repair" ]; then
         monrun $ok "OK, virtual host, skip checking"
      fi
      exit 0
   fi
fi

if [ $(whoami) != 'root' ] ; then
  monrun $warn 'Please use "sudo" or become "root" to run this script.'
fi

get_system_disks() {
  if ! root_dev=$(findmnt -nc -o SOURCE /); then
    root_dev=$(readlink -m `awk '/\/dev[^ ]+ \/ / {print $1}' /proc/mounts`)
    if [ -z "$root_dev" ];then
      root_dev=`df / | grep -o '/dev/[^[:space:]]\+'`
    fi
  fi
  if [ -z "$root_dev" ]; then
    monrun $warn "Can't find root device!"
  elif [[ "$root_dev" =~ mapper ]]; then
    monrun $ok "Look like virtual host, skip check!"
    exit 0
  fi
  #blkid -g; # Clear blkid cache

  case ${root_dev#/dev/} in
    md*)
      root_dev_md_uuid=`mdadm -D $root_dev|awk '/UUID/{gsub(/:/,i,$NF);print $NF}'`
      SYSDisks=(`sed -n "s|${root_dev#/dev/} : \w\+ \w\+ ||p" /proc/mdstat|\
                      xargs -n1 echo | sed '/sd[a-z]/s/[0-9]\+\[[0-9]\+\]//; /nvme/s/p[0-9]\+\[[0-9]\+\]//'`);;
    sd*) SYSDisks=( `echo ${root_dev//[0-9]}` );;
  esac
}

# The Grub2 (v1.99) core_dir string is contained in a LZMA stream.
if type xz 1>&2; then
   debug "xz available"
   UNLZMA='xz --format=lzma --decompress'
elif type lzma 1>&2; then
   debug "lzma available"
   UNLZMA='lzma -cd'
else
   monrun $crit "xz or lzma isn't available"
fi


## Grub Legacy ##
# Determine the embeded location of stage2 in a stage1 file, look for the stage2
# and, if found, determine the location and the path of the embedded menu.lst.
stage2_loc () {
   local stage1="$1" HI _t
   offset=$(hexdump -v -s 68 -n 4 -e '4 "%u"' "$stage1");
   dr=$(hexdump -v -s 64 -n 1 -e '1/1 "%u"' "$stage1");
   pa="no_stage_2_file"; Grub_Version='';

   for HI in "${!SYSDisks[@]}"; do
      hdd="/dev/${SYSDisks[HI]}";
      if [ $offset -lt ${HDSize[${SYSDisks[HI]}]} -a "$hdd" = "$stage1" ]; then
         _t=$(dd if=$hdd skip=$offset count=1|hexdump -v -n 4 -e '"%x"');
         debug "Looks for $HI drive $hdd Grub Legacy's stage2 - offset $offset, magic $_t"
         if [[ "$_t" =~ ^(3be5652|bf5e5652)$ ]]; then # stage2 files were found.
            debug "Stage2 file found (finger print $_t)"
            dd if=$hdd skip=$((offset + 1)) count=1 of=$tfile;
            pa=$(hexdump -v -s 10 -n 1 -e '"%d"' $tfile);
            stage2_hdd=$hdd;
            Grub_String=$(hexdump -v -s 18 -n 94 -e '"%_u"' $tfile);
            Grub_Version=$(echo $Grub_String|sed 's/nul[^$]*//');
            BL="$BL $Grub_Version";
            menu=$(echo $Grub_String|sed 's/[^\/]*//;s/nul[^$]*//');
            menu=${menu%% *};
         fi
      fi
   done

   dr=$((${dr}-127));
   Stage2_Msg="looks at sector ${offset}";

   if [ "${dr}" -eq 128 ] ; then
      Stage2_Msg="${Stage2_Msg} of the same hard drive";
   else
      Stage2_Msg="${Stage2_Msg} on boot drive #${dr}";
   fi

   Stage2_Msg="${Stage2_Msg} for the stage2 file";

   if [ "${pa}" = "no_stage_2_file" ] ; then
      # no stage 2 file found.
      debug "No stage2 file found"
      ERROR_Level=$crit
      Stage2_Msg="${Stage2_Msg}, but no stage2 files can be found at this location.";
   else
      ERROR_Level=$ok
      pa=$((${pa}+1));
      Stage2_Msg="${Stage2_Msg}.  A stage2 file is at this location on ${stage2_hdd}.  Stage2 looks on";
      if [ "${pa}" -eq 256 ] ; then
         Stage2_Msg="${Stage2_Msg} the same partition";
      else
         Stage2_Msg="${Stage2_Msg} partition #${pa}";
      fi
      Stage2_Msg="${Stage2_Msg} for ${menu}";
   fi
}

## Grub2 ##
#
#   Collect fragments of core.img using information encoded in the first
#   block (diskboot.img).
#

grub2_read_blocklist () {
  local hdd="$1";
  local core_img_file="$2";
  local sector_nr fragment_size fragment_offset=1 block_list=500;

  # Assemble fragments from "hdd" passed to grub2_info.
  # Each block list entry is 12 bytes long and consists of
  #   8 bytes = fragment start absolute disk offset in sectors
  #   2 bytes = fragment size in sectors
  #   2 bytes = memory segment to load fragment into
  # Entries start at the end of the first sector of core.img and
  # go down. End marker is all zeroes.

  while [ ${block_list} -gt 12 ]; do
    sector_nr=$(hexdump -v -n 8 -s ${block_list} -e '1/8 "%u"' ${core_img_file});
    if [ ${sector_nr} -eq 0 ];then return; fi
    fragment_size=$(hexdump -v -n 2 -s $((block_list+8)) -e '1/2 "%u"' ${core_img_file});
    dd if="${hdd}" of=${core_img_file} skip=${sector_nr} \
      seek=${fragment_offset} count=${fragment_size}|| return;
    let "fragment_offset += fragment_size";
    let "block_list -= 12";
  done
}

## Grub2 ##
#
#   Determine the (embeded) location of core.img for a Grub2 boot.img file,
#   determine the path of the grub2 directory and look for an embedded config file.
#

grub2_info () {
   local stage1="$1" hdd="$2"
   # When $grub2_version is "1.99-2.00", we want to override this value
   # with a more exact value later (needs to be a global variable).
   grub2_version="$3";
   # Have we got plain file or need to collect full core.img from blocklists?
   local core_source="$4";

   local sector_offset drive_offset dir_offset sector_nr drive_nr drive_nr_hex;
   local partition core_dir embedded_config HI magic core_img_found=0 embedded_config_found=0;
   local total_module_size kernel_image_size compressed_size offset_lzma lzma_uncompressed_size;
   local grub_module_info_offset grub_module_magic grub_modules_offset grub_modules_size;
   local grub_module_type grub_module_size grub_module_header_offset grub_modules_end_offset;
   local lzma_compressed_size reed_solomon_redundancy reed_solomon_length boot_dev boot_drive;
   local core_img_flavour='detect';
    :> $core_img_file_type_2

   case "${grub2_version}" in
      1.96) sector_offset='68';  drive_offset='76'; dir_offset='553';;
      1.97-1.98) sector_offset='92';  drive_offset='100'; dir_offset='540';;
      1.99|1.99-2.00|2.00) sector_offset='92';  drive_offset='100';;
   esac
   # Offset to core.img (in sectors).
   sector_nr=$(hexdump -v -s $sector_offset -n 4 -e '4 "%u"' "$stage1");
   # BIOS drive number on which grub2 looks for its second stage (=core.img):
   #   - "0xff" means that grub2 will use the BIOS drive number passed via the DL register.
   #   - if this value isn't "0xff", that value will used instead.
   drive_nr_hex=$(hexdump -v -s $drive_offset -n 1 -e '"0x%02x"' "$stage1");
   drive_nr=$((drive_nr_hex - 127));
   Grub2_Msg="looks at sector ${sector_nr} on $hdd for core.img";

   for HI in "${!SYSDisks[@]}"; do
      # If the drive name passed to grub2_info matches the drive name of the current
      # value of SYSDisks, see if the sector offset to core.img is smaller than the
      # total number of sectors of that drive.
      _hdd=${SYSDisks[HI]}
      if [ "x$hdd" = "x/dev/$_hdd" -a $sector_nr -lt ${HDSize[$_hdd]} ]; then
         if [ "$core_source" = 'file' ] ; then # Use "file" passed to grub2_info directly.
            dd if="$stage1" of=$core_img_file skip=$sector_nr count=1024
         else # Use "hdd" passed to grub2_info.
            dd if="$hdd" of=$core_img_file skip=$sector_nr count=1024
         fi
         if [ ! -s "$core_img_file" -a -z "$_repair" ];then
            # spam stdout in check mode for "can't parse"
            echo "Can't write $core_img_file, run $0 -d for debug"
            continue
         fi
         magic=$(hexdump -v -n 4 -e '/1 "%02x"' $core_img_file);
         debug "Found magic $magic in file $core_img_file at sector_nr:$sector_nr
                           on hdd $hdd with size ${HDSize[$_hdd]}"
         # 52bff481 - RHEL7 diskboot.S with patched out message
         if [[ "${magic}" =~ ^52(56be1b|56be6f|e82801|bff481)$ ]]; then
            debug "Found core.img"
            # core.img file was found.
            core_img_found=1;

            if [[ ${grub2_version} =~ ^(1\.99(-2\.00)?|2\.00)$ ]]; then
               # Find the last 8 bytes of lzma_decode to find the offset of the lzma_stream:
               #   - v1.99: "d1 e9 df fe ff ff 00 00"
               #   - v2.00: "d1 e9 df fe ff ff 66 90" (pad bytes NOP)
               #            "d1 e9 df fe ff ff 8d"    (pad bytes LEA ...)
               #
               # arvidjaar@gmail.com:
               #  final directive in startup_raw.S is .p2align 4 which (at least using
               #  current GCC/GAS) adds lea instructions (8d...). Exact format and length
               #  apparently depend on pad size and may be on toolkit version. So just
               #  accept anything starting with lea.
               # FIXME what if it ends on exact 16 byte boundary?
               eval $(hexdump -v -n 10000 -e '1/1 "%02x"' $core_img_file|awk '{
                  found_at=match($0,"d1e9dffeffff");
                  if(found_at == "0"){print "offset_lzma=0";}else{
                  print "offset_lzma="(found_at-1)/2+8";lzma_decode_last8bytes="substr($0,found_at,16)";"}}');
               if [ "${grub2_version}" = '1.99-2.00' ] ; then
                  if [[ "$lzma_decode_last8bytes" =~ ^d1e9dffeffff(6690|8d..)$ ]]
                     then grub2_version='2.00';
                     else grub2_version='1.99';
                  fi
               fi
            else
               # Grub2 (v1.96 and v1.97-1.98).
               partition=$(hexdump -v -s 532 -n 1 -e '"%d"' ${core_img_file});
               core_dir=$(hexdump -v -s $dir_offset -n 64 -e '"%_u"' $core_img_file|sed 's/nul[^$]*//');
            fi

            if [ "${grub2_version}" = '1.99' ] ; then
               # For Grub2 (v1.99), the core_dir is just at the beginning of the compressed part of core.img.
               #
               # Get grub_total_module_size	: byte 0x208-0x20b of embedded core.img ==> byte 520
               # Get grub_kernel_image_size	: byte 0x20c-0x20f of embedded core.img ==> byte 524
               # Get grub_compressed_size	: byte 0x210-0x213 of embedded core.img ==> byte 528
               # Get grub_install_dos_part	: byte 0x214-0x218 of embedded core.img ==> byte 532
               #                             --> only 1 byte needed (partition)
               eval $(hexdump -v -s 520 -n 13 -e \
                  '1/4 "total_module_size=%u; " 1/4 "kernel_image_size=%u; " 1/4 "compressed_size=%u; " 1 "partition=%d;"' \
                  $core_img_file);

               # XXX В новом скрипте этого нет
               # Scan for "d1 e9 df fe ff ff 00 00": last 8 bytes of lzma_decode to find the offset of the lzma_stream.
               #eval $(hexdump -v -n ${kernel_image_size} -e '1/1 "%02x"' ${core_img_file} | \
                  #  awk '{found_at=match($0, "d1e9dffeffff0000");if(found_at == "0"){
               #         print "offset_lzma=0" }else{
               #         print "offset_lzma="((found_at-1)/2)+8}}');

               # Do we have xz or lzma installed?
               if [ $offset_lzma -ne 0 ] ; then
                  # Correct the offset to the lzma stream, when 8 subsequent
                  # bytes of zeros are at the start of this offset,
                  if [[ $(hexdump -v -s $offset_lzma  -n 8 -e '1/1 "%02x"' $core_img_file) =~ ^0{16}$ ]]
                     then offset_lzma=$((offset_lzma + 8));
                  fi
                  # Calculate the uncompressed size to which the compressed lzma stream needs to be expanded.
                  lzma_uncompressed_size=$((total_module_size + kernel_image_size - offset_lzma + 512));
                  # Make lzma header (13 bytes): ${lzma_uncompressed_size} must be displayed in little endian format.
                  printf '\x5d\x00\x00\x01\x00'$(printf '%08x' $((lzma_uncompressed_size - offset_lzma + 512))\
                     |awk '{printf "\\x%s\\x%s\\x%s\\x%s",substr($0,7,2),substr($0,5,2),
                           substr($0,3,2),substr($0,1,2)}')'\x00\x00\x00\x00' >$tfile;
                  # Get lzma_stream, add it after the lzma header and decompress it.
                  dd if=$core_img_file bs=$offset_lzma skip=1 count=$((lzma_uncompressed_size / offset_lzma + 1))\
                     |cat $tfile - |$UNLZMA > $core_img_file_unlzma

                  # Get core dir.
                  core_dir=$(hexdump -v -n 64 -e '"%_c"' $core_img_file_unlzma);
                  # Remove "\0"s at the end.
                  core_dir="${core_dir%%\\0*}";

                  # Offset of the grub_module_info structure in the uncompressed part.
                  grub_module_info_offset=$((kernel_image_size - offset_lzma + 512));

                  eval $(hexdump -v -n 12 -s $grub_module_info_offset -e \
                     '"grub_module_magic=" 4/1 "%_c" 1/4 "; grub_modules_offset=%u; " 1/4 "grub_modules_size=%u;"' \
                     $core_img_file_unlzma);

                  # Check for the existence of the grub_module_magic.
                  if [ x"${grub_module_magic}" = x'mimg' ] ; then
                     # Embedded grub modules found.
                     grub_modules_end_offset=$((grub_module_info_offset + grub_modules_size));
                     grub_module_header_offset=$((grub_module_info_offset + grub_modules_offset));

                     # Traverse through the list of modules and check if it is a config module.
                     while [ $grub_module_header_offset -lt $grub_modules_end_offset ] ; do
                        eval $(hexdump -v -n 8 -s $grub_module_header_offset -e \
                           '1/4 "grub_module_type=%u; " 1/4 "grub_module_size=%u;"' \
                           $core_img_file_unlzma);
                        if [ ${grub_module_type} -eq 2 ] ; then
                           # This module is an embedded config file.
                           embedded_config_found=1;
                           embedded_config=$(hexdump -v -n $((grub_module_size - 8)) -s \
                              $((grub_module_header_offset + 8)) -e '"%_c"' $core_img_file_unlzma);
                           # Remove "\0" at the end.
                           embedded_config=$( printf "${embedded_config%\\0}" );
                           break;
                        fi
                        grub_module_header_offset=$((grub_module_header_offset + grub_module_size));
                     done
                  fi
               fi
            elif [ "$grub2_version" = '2.00' ] ; then
               # First make sure to collect core.img fragments if it is
               # embedded.
               if [[ "$core_source" =~ ^(disk|partition)$ ]]; then
                  grub2_read_blocklist "$hdd" $core_img_file
               fi

               # For Grub2 (v2.00), the core_dir is stored in the compressed part of core.img in the same
               # way as the modules and embedded config file.

               # Get grub_compressed_size	        : byte 0x208-0x20b of embedded core.img => byte 520
               # Get grub_uncompressed_size	      : byte 0x20c-0x20f of embedded core.img => byte 524
               # Get grub_reed_solomon_redundancy : byte 0x210-0x213 of embedded core.img => byte 528
               # Get grub_no_reed_solomon_length  : byte 0x214-0x217 of embedded core.img => byte 532
               # Get grub_boot_dev		     : byte 0x218-0x21a of embedded core.img => byte 536
               #                             ( should also contain the grub_boot_drive field )
               # Get grub_boot_drive		   : byte 0x21b of embedded core.img => byte 539
               eval $(hexdump -v -s 520 -n 20 -e \
                  '1/4 "lzma_compressed_size=%u; " 1/4 "lzma_uncompressed_size=%u; " 1/4 "reed_solomon_redundancy=%u; " 1/4 "reed_solomon_length=%u; boot_dev=" 3/1 "%x" 1 "; boot_drive=%d;"' \
                  $core_img_file);

               # Do we have xz or lzma installed?
               if [ $offset_lzma -ne 0 ] ; then
                  # Grub2 pads the start of the lzma stream to a 16 bytes boundary.
                  # Correct the offset to the lzma stream if necessary
                  # Current GCC adds lea instructions as pad bytes
                  padsize=$(((((offset_lzma + 15) >> 4) << 4) - offset_lzma));
                  if [ $padsize -gt 0 ] ; then
                     offset_lzma=$((offset_lzma + padsize));
                  fi
                  # Make lzma header (13 bytes): ${lzma_uncompressed_size} must be displayed in little endian format.
                  printf '\x5d\x00\x00\x01\x00'$(printf '%08x' $lzma_uncompressed_size\
                     |awk '{printf "\\x%s\\x%s\\x%s\\x%s",substr($0,7,2),substr($0,5,2),
                        substr($0,3,2),substr($0,1,2)}')'\x00\x00\x00\x00' >$tfile;

                  # Get lzma_stream, add it after the lzma header and decompress it.
                  dd if=$core_img_file bs=$offset_lzma skip=1 count=$lzma_compressed_size\
                     |cat $tfile - |$UNLZMA >$core_img_file_unlzma;

                  # Get offset to the grub_module_info structure in the uncompressed part.
                  eval $(hexdump -v -s 19 -n 4 -e '1/4 "grub_module_info_offset=%u;"' $core_img_file_unlzma);

                  eval $(hexdump -v -n 12 -s $grub_module_info_offset -e \
                     '"grub_module_magic=" 4/1 "%_c" 1/4 "; grub_modules_offset=%u; " 1/4 "grub_modules_size=%u;"' \
                     $core_img_file_unlzma);

                  # Check for the existence of the grub_module_magic.
                  if [ x"${grub_module_magic}" = x'mimg' ] ; then
                     # Embedded grub modules found.
                     grub_modules_end_offset=$((grub_module_info_offset + grub_modules_size));
                     grub_module_header_offset=$((grub_module_info_offset + grub_modules_offset));

                     # Traverse through the list of modules and check if it is a config module.
                     # Upstream GRUB2 supports following module types:
                     #   0 - ELF modules; may be included multiple times
                     #   1 - memory disk image; should be included just once
                     #   2 - embedded initial configuration code; should be included just once
                     #   3 - initial value of ${prefix} variable. Device part may be omitted,
                     #       in which case device is guessed at startup
                     #   4 - public GPG keyring used for file signature checking;
                     #       may be included multiple times
                     #
                     # All parts are optional (although in practice at least drivers
                     # for disk and filesystem must be present).
                     #
                     # Since RPM version 2.00-10 fedora includes patch that inserts additional
                     # module type after the first one, thus shifting all numbers starting
                     # with 1. So embedded config and prefix become 3 and 4 on fedora.
                     while [ $grub_module_header_offset -lt $grub_modules_end_offset ] ; do
                        eval $( hexdump -v -n 8 -s $grub_module_header_offset -e \
                           '1/4 "grub_module_type=%u; " 1/4 "grub_module_size=%u;"' \
                           $core_img_file_unlzma);

                        if [ $grub_module_type -eq 1 ] ; then
                           # "stale" ELF module on fedora or memory disk everywhere else
                           if [ $core_img_flavour = 'detect' ] ; then
                              if [ "$(hexdump -v -n 4 -s $((grub_module_header_offset + 8)) \
                                 -e '"%c"'  $core_img_file_unlzma)" = $'\x7f''ELF' ]; then
                                 core_img_flavour='fedora'; # fedora "stale" ELF module
                              else
                                 core_img_flavour='upstream';
                              fi
                           fi
                        elif [ $grub_module_type -eq 2 ] ; then
                           # memory disk on fedora or embedded config everywhere else
                           if [ $core_img_flavour = 'detect' ] ; then
                              # Normally core.img will have prefix which is easier to detect,
                              # so leave detection as last resort.
                              dd if=$core_img_file_unlzma of=$core_img_file_type_2 \
                                 bs=1 skip=$((grub_module_header_offset + 8)) \
                                 count=$((grub_module_size - 8))
                           fi
                           if [ $core_img_flavour = 'upstream' ] ; then
                              # This module is an embedded config file.
                              embedded_config_found=1;
                              # Remove padding "\0"'s at the end.
                              embedded_config=$(hexdump -v -n $((grub_module_size - 8)) \
                                 -s $((grub_module_header_offset + 8)) -e '"%_c"' \
                                    $core_img_file_unlzma | sed -e 's/\(\\0\)\+$//');
                           fi
                        elif [ $grub_module_type -eq 3 ] ; then
                           # embedded config on fedora or prefix everywhere else
                           if [ ${core_img_flavour} = 'detect' ] ; then
                              # if it looks like file name, assume prefix
                              if [[ "$(hexdump -v -n 1 -s $((grub_module_header_offset + 8)) \
                                 -e '"%c"' $core_img_file_unlzma)" == [/\(] ]] ; then
                                 core_img_flavour='upstream';
                              else
                                 core_img_flavour='fedora';
                              fi
                           fi
                           if [ $core_img_flavour = 'upstream' ] ; then
                              # This module contains the prefix.
                              # Get core dir. Remove padding "\0"'s at the end.
                              core_dir=$( hexdump -v -n $((grub_module_size - 8)) -s \
                                 $((grub_module_header_offset + 8)) -e '"%_c"' \
                                    $core_img_file_unlzma|sed -e 's/\(\\0\)\+$//');
                           elif [ $core_img_flavour = 'fedora' ] ; then
                              # This module is an embedded config file.
                              embedded_config_found=1;
                              # Remove padding "\0"'s at the end.
                              embedded_config=$( hexdump -v -n $((grub_module_size - 8)) \
                                 -s $((grub_module_header_offset + 8)) -e '"%_c"' \
                                 $core_img_file_unlzma|sed -e 's/\(\\0\)\+$//');
                           fi
                        elif [ ${grub_module_type} -eq 4 ] ; then
                           # prefix on fedora or GPG keyring everywhere else
                           if [ ${core_img_flavour} = 'detect' ] ; then
                              # if it looks like file name, assume prefix
                              # GPG ring normall has \x99 as first byte
                              if [[ "$(hexdump -v -n 1 -s $((grub_module_header_offset+8)) \
                                 -e '"%c"' $core_img_file_unlzma)" == [/\(] ]]; then
                                 core_img_flavour='fedora';
                              else
                                 core_img_flavour='upstream';
                              fi
                           fi
                           if [ ${core_img_flavour} = 'fedora' ] ; then
                              # This module contains the prefix.
                              # Get core dir.  Remove padding "\0"'s at the end.
                              core_dir=$( hexdump -v -n $((grub_module_size - 8)) -s \
                                 $((grub_module_header_offset + 8)) -e '"%_c"' \
                                 $core_img_file_unlzma|sed -e 's/\(\\0\)\+$//');
                           elif [ $core_img_flavour = 'upstream' ] ; then
                              : # TODO list GPG keyring
                           fi
                        fi
                        grub_module_header_offset=$((grub_module_header_offset + grub_module_size));
                     done
                  fi
               fi
            else
               # Grub2 (v1.96 and v1.97-1.98).
               partition=$(hexdump -v -s 532 -n 1 -e '"%d"' $core_img_file);
               core_dir=$(hexdump -v -s $dir_offset -n 64 -e \
                              '"%_u"' $core_img_file|sed 's/nul[^$]*//');
            fi
         fi
      fi
   done

   if [ $core_img_found -eq 0 ] ; then # core.img not found.
      ERROR_Level=$crit
      Grub2_Msg="${Grub2_Msg}, but core.img can not be found at this location";
   else # core.img found.
      ERROR_Level=$ok
      if [ "$drive_nr_hex" != '0xff' ] ; then
         Grub2_Msg="${Grub2_Msg}. Grub2 is cfg load core.img from BIOS drive ${drive_nr} \
            (${drive_nr_hex}) instead of using the boot drive passed by the BIOS";
      fi
      Grub2_Msg="${Grub2_Msg}. core.img is at this location"
      partition=$((partition+1));
      if [ $embedded_config_found -eq 0 ]; then # No embedded config file found.
         if [ ${partition} -eq 255 ] ; then
            if [[ "$core_dir" =~ mduuid/([a-f0-9]+) ]];then
               if [[ "x$root_dev_md_uuid" == "x${BASH_REMATCH[1]}" ]];then
                  Grub2_Msg="$Grub2_Msg and looks for $core_dir on drive $root_dev"
               else
                  ERROR_Level=$crit
                  Grub2_Msg="$Grub2_Msg and looks for $core_dir on NOT ROOT drive (root mduuid: ${root_dev_md_uuid})!"
               fi
            fi
         else
            Grub2_Msg="$Grub2_Msg and looks in partition $partition for $core_dir"
         fi
      else # Embedded config file found.
         Grub2_Msg="$Grub2_Msg and uses an embedded config file"
         # XXX: нужно ли показывать встроенный конфиг, если он неожиданно найден?
         # $embedded_config
      fi
   fi
}

get_system_disks

for hi in "${!SYSDisks[@]}"; do
   _size_in_blocks=`< /sys/block/${SYSDisks[hi]}/size`
   _block_size=`< /sys/block/${SYSDisks[hi]}/queue/physical_block_size`

   HDSize[${SYSDisks[hi]}]=$((_size_in_blocks * _block_size))
   debug "$hi disk:${SYSDisks[hi]} has size ${HDSize[${SYSDisks[hi]}]} bytes"
done

## Identify the MBR of each hard drive. ##
for hi in "${!SYSDisks[@]}"; do
   ERROR_Level=$ok # start with ERROR_Level=ok
   drive="/dev/${SYSDisks[hi]}"; [ -e $drive ] || continue
   debug "Check disk:$drive"

   # Read the whole MBR in hexadecimal format.
   MBR_512=$(hexdump -v -n 512 -e '/1 "%02x"' ${drive});

   ## Look at the first 2,3,4 or 8 bytes of the hard drive to identify the boot code installed in the MBR. ##
   #   If it is not enough, look at more bytes.
   MBR_sig2="${MBR_512:0:4}";

   ## Bytes 0x80-0x81 of the MBR. ##
   #   Use it to differentiate between different versions of the same bootloader.
   MBR_bytes80to81="${MBR_512:256:4}";

   Message="";
   case ${MBR_sig2} in
      eb48)
         debug "## Grub Legacy is in the MBR. ##"
         BL="Grub Legacy"
         # 0x44 contains the offset to the next stage.
         offset=$(hexdump -v -s 68 -n 4 -e '"%u"' ${drive});
         if [ "${offset}" -ne 1 ] ; then
            debug "Grub Legacy is installed without stage1.5 files."
            stage2_loc ${drive};
            Message="${Message} and ${Stage2_Msg}";
         else
            debug "Grub Legacy is installed with stage1.5 files."
            Grub_String=$(hexdump -v -s 1042 -n 94 -e '"%_u"' ${drive});
            Grub_Version="${Grub_String%%nul*}";
            BL="Grub Legacy (v${Grub_Version})";

            tmp="/${Grub_String#*/}";
            tmp="${tmp%%nul*}";
            eval $(echo $tmp|awk '{ print "stage=" $1 "; menu=" $2 }');
            [[ x"$menu" = x'' ]] || stage="${stage} and ${menu}";
            part_info=$((1045 + ${#Grub_Version}));
            eval $(hexdump -v -s $part_info -n 2 -e '1/1 "pa=%u; " 1/1 "dr=%u"' $drive);
            dr=$((dr - 127)); pa=$((pa + 1));
            if [ "${dr}" -eq 128 ] ; then
               Message="${Message} and looks on the same drive in partition #${pa} for ${stage}";
            else
               Message="${Message} and looks on boot drive #${dr} in partition #${pa} for ${stage}";
            fi
         fi;;
      eb4c)
         debug "## Grub2 (v1.96) is in the MBR. ##"
         BL='Grub2 (v1.96)';
         grub2_info $drive $drive '1.96' 'drive';
         Message="$Message and $Grub2_Msg";;
      eb63)
         debug "## Grub2 is in the MBR. ##"
         case $MBR_bytes80to81 in
            7c3c) grub2_version='1.97-1.98'; BL='Grub2 (v1.97-1.98)';;
            0020) grub2_version='1.99-2.00'; BL='Grub2 (v1.99-2.00)';;
         esac
         grub2_info $drive $drive $grub2_version 'drive';
         BL="Grub2 (v${grub2_version})";
         Message="$Message and $Grub2_Msg";;
      *)
         BL='Unknown boot loader';
         ERROR_Level=$crit
   esac
   info "=> $drive has Boot Loader $BL\n $Message"
   if [ $ERROR_Level -gt $ok ]; then
      debug "## Bad Boot Loader on Hard Drive $drive: $BL ##"
      BAD_HDBL[${SYSDisks[hi]}]="$BL"
   else
      HDBL[${SYSDisks[hi]}]="$BL"
   fi
done

unset MONRUN_Message
if [ ${#BAD_HDBL[@]} -gt 0 ]; then
   debug "Override ERROR_Level $ERROR_Level -> $crit"
   ERROR_Level=$crit
fi

for hdd in ${!BAD_HDBL[@]}; do
   MONRUN_Message="${MONRUN_Message}$hdd (broken!!!):${BAD_HDBL[$hdd]}, "
done
debug "Bad Loader monrun msg: '$MONRUN_Message'"

for hdd in ${!HDBL[@]}; do
   MONRUN_Message="${MONRUN_Message}$hdd:${HDBL[$hdd]}, "
done
debug "Result monrun msg: '${MONRUN_Message%, }' and level: $ERROR_Level"

repair
monrun $ERROR_Level "${MONRUN_Message%, }"

exit 0
