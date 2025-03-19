# Windows Server builds

## tests

* winrm (dont forget -> used cert CN=... == hostname)
  * firewall open for https
* certificates must be clean (Machine key regenerates, no PKs preserved so 'em useless!)
* fs
  * no bootstrap folder
  * clean setupcomplete/scripts folder
  * ! temp files must exist (были несчастные случаи на производстве)
  * ! recyclebin clean
  * ! remove sysprep.xml
* activation
  * activates
    * kms server correct (registry :-\)
    * client key correct
* no hard shutdown messages
* cloudbase-init works
  * config is correct
* smb1 is not installed
* eth's named correctly
* 'Administrator' password never expires
* task scheduler '/' is clean
* rdp allowed
* nla is turned on
* realtimeisuniversal for sure
* serial console enabled
* power plan is highperformance
* shutdown without logon is allowed
* icmp allowed

'!' think abouts

## steps

* bootstrap
* cloudbase-init
* activation (only to set kms server :p)
* seal
  * setupcomplete
    * at forge:
      * clean setupcomplete folder (there's leftover from qemu-builder, fix need's to be merged)
      * sysprepunattend.xml with cloudbase-init
      * copy common
      * copy setupcompletes
      * removes userdata task in scheduler
      * cleanup
        * downloaded updates
        * panther logs (sysprep logs)
        * temp files
        * WER reports
        * recycle bin
        * windows logs
        * windows eventlogs
        * vss
        * disable defrag task in scheduler
        * remove bootstrap
      * runs sysprep
      * shutdown
    * at 1st run: (unordered)
      * setupcomplete.cmd:
        * runs setupcomplete.ps1
        * deletes itself
      * setupcomplete.ps1
        * winrm
        * activation (one-shot, not working? it'll retry once a day)
        * admin password never expire
        * eth naming
        * packer tasks removed from scheduler - known bug
        * deletes everything except setupcomplete.cmd  
* wait 5m (noop 'echo "faaaart in the waaaater"', sometimes windows got killed, but we are chaotic good, so...)

## note

Use `{ "type": "shell-local", "inline": ["echo \"pwd is {{ user `generated_password` }}\""] },` for debug