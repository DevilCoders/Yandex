if [ "x$LC_USER" != "x" -a -f /home/$LC_USER/.bashrc_salt ]; then
    . /home/$LC_USER/.bashrc_salt
    return
fi

if [ "x$LC_USER" != "x" -a -f /home/$LC_USER/.bashrc ]; then
    . /home/$LC_USER/.bashrc
    return
fi

if [ "x$SUDO_USER" != "x" -a -f /home/$SUDO_USER/.bashrc ]; then
    . /home/$SUDO_USER/.bashrc
    return
fi

###
###  below is bashrc from config-yandex-friendly-bash
###

domain=`hostname -f | rev | cut -d. -f 1,2,3 | rev`
hostname=`hostname -f | sed s/$domain// | sed 's/\.$//'`

export MYSQL_PS1="\u@"$hostname" [\d]> "

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# make less more friendly for non-text input files, see lesspipe(1)
[ -x /usr/bin/lesspipe ] && eval "$(lesspipe)"

# set variable identifying the chroot you work in (used in the prompt below)
if [ -z "$debian_chroot" -a -r /etc/debian_chroot ]; then
    debian_chroot=$(cat /etc/debian_chroot)
fi

# check if there are roots nearby
root_count=`who --ips | awk '{print $NF}' | sort | uniq | wc -l`
maint_count=`ps axo comm | egrep '^(nc|nc.traditional)$' | wc -l`
if [ $root_count -gt 1 -o $maint_count -gt 0 ]; then
    echo '****************************************************************'
    echo 'This host is most likely under maintenance. Please, investigate.'
    echo '****************************************************************'
    echo This is the reason:
    if [ $root_count -gt 1 ]; then who; fi
    if [ $maint_count -gt 0 ]; then ps axo comm | egrep '^(nc|nc.traditional)$' | sort | uniq | while read n; do echo $n is in the process list; done; fi
    echo
fi

HOST=`hostname -f | cut -f1 -d.`
DOMAIN=`hostname -f | cut -f2 -d.`
COLOR_HOST="38;5;27"
COLOR_DOMAIN="38;5;102"
[ -f ~/.prompt_colors ] && . ~/.prompt_colors
if [ "X$OVERRIDE_PS1" != "X" ]; then
        export PS1=$OVERRIDE_PS1
else
    case "$TERM" in
    xterm*)
        export PS1="\[\e[${COLOR_HOST}m\]${HOST}\[\e[${COLOR_DOMAIN}m\].${DOMAIN}\[\e[0m\]:\w\\$ "
        PS1="\[\033]2;${HOST}.${DOMAIN}:\w\007\]$PS1"
        ;;
    *)
		PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
        ;;
	esac
fi

# If this is an xterm set the title to user@host:dir
case "$TERM" in
xterm*|rxvt*)
    PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\007"'
    ;;
*)
    ;;
esac

# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.

#if [ -f ~/.bash_aliases ]; then
#    . ~/.bash_aliases
#fi

# enable color support of ls and also add handy aliases
if [ "$TERM" != "dumb" ]; then
    eval "`dircolors -b`"
    alias ls='ls -v --color=auto'
fi

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
fi
