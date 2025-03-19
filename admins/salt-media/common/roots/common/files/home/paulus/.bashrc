# Exit when the session is non-interactive
[ -z "$PS1" ] && return

# Generic options and aliases
export HISTCONTROL=ignoredups
export HISTSIZE=1000
export HISTFILESIZE=2000
shopt -s checkwinsize
shopt -s histappend
shopt -s globstar

alias ls='ls --color=auto'
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'
alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'
alias highstate='sudo salt-call -l critical --retcode-passthrough --output-diff --force-color state.highstate queue=True'

# Extra colors and such
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
fi
[ -x /usr/bin/lesspipe ] && eval "$(lesspipe)"
[ -f /etc/bash_completion ] && . /etc/bash_completion

# Colors and extra vars
HOSTNAME=`hostname -f`
HOST=${HOSTNAME/.*}
DOMAINNAME=${HOSTNAME/$HOST.}
DOMAIN=${DOMAINNAME/.*}
COLOR_DOMAIN="30;1"
COLOR_TAGS="0"
COLOR_SCREEN="33;1"

[ -f /etc/yandex/environment.type ] && ENV=`cat /etc/yandex/environment.type`
case "$ENV" in
    production)
        COLOR_HOST="31;1"
        ITERM_COLOR=(214 93 93)
        ;;
    testing)
        COLOR_HOST="33;1"
        ITERM_COLOR=(214 147 93)
        ;;
    development)
        COLOR_HOST="32;1"
        ITERM_COLOR=(93 147 93)
        ;;
    *)
        COLOR_HOST="34;1"
        ITERM_COLOR=(93 147 214)
        ;;
esac

# Check root
EXTRALOG=()
if [ `id -u` -eq 0 ]; then
    EXTRALOG+=("You have logged in as \e[31;1mroot\e[0m.  ===> Please consider switching to a regular user.")
    TAGS+=("\[\e[31;1m\]root\[\e[0m\]")
fi

# Extra users
for u in `who | fgrep -v $USER | awk '{print \$1}' | sort | uniq`; do
    EXTRALOG+=("\e[${COLOR_SCREEN}m${u}\e[0m is logged in.")
done

# Mongo
if [ -x /usr/bin/mongo ]; then
    if [ -e /etc/mongodb.conf ]; then
        mongo_file=/etc/mongodb.conf
        mongo_password_prefix=M_GRANTS_RC_mongodb
        default_mongo_port=27018
    elif [ -e /etc/mongodbcfg.conf ]; then
        mongo_file=/etc/mongodbcfg.conf
        mongo_password_prefix=M_GRANTS_RC_mongodb
        default_mongo_port=27019
    elif [ -e /etc/mongos.conf ]; then
        mongo_file=/etc/mongos.conf
        mongo_password_prefix=M_GRANTS_RC_mongos
        default_mongo_port=27017
        is_mongos=1
    fi

    if [ "x$mongo_file" != "x" ]; then
        mongo_port=$(grep -Po "port\s*\=\s*\K[0-9]+" ${mongo_file})
        if [ "x$mongo_port" = "x" ]; then mongo_port=$default_mongo_port; fi
        if sudo -n stat /root/.mongorc.js &>/dev/null; then
            mongo_pass=$(sudo -n cat /root/.mongorc.js | grep $mongo_password_prefix | grep -Po "auth\('admin','\K[^']+")
            alias mongo="mongo --port $mongo_port --username admin --password '$mongo_pass' --authenticationDatabase admin"
            alias mongostat="mongostat --port $mongo_port --username admin --password '$mongo_pass' --authenticationDatabase admin"
            alias mongotop="mongotop --port $mongo_port --username admin --password '$mongo_pass' --authenticationDatabase admin"
        fi
    fi

    if [ "x$is_mongos" != "x" ]; then
        TAGS+=("mongos")
    else
        read OK PRIMARY RS <<< $(mongo --norc --quiet --eval "x=rs.isMaster();print('ok');print(x.primary);print(x.setName)" admin 2>/dev/null)
        if [ "x$OK" = "xok" ]; then
            PRIMARY=${PRIMARY/:*}
            if [ "x$PRIMARY" != "x" ]; then
                if [ "x$PRIMARY" = "x$HOSTNAME" ]; then
                    EXTRALOG+=("This is a \e[${COLOR_SCREEN}mprimary\e[0m.")
                else
                    EXTRALOG+=("The primary is at \e[${COLOR_SCREEN}m$PRIMARY\e[0m.")
                fi
                RS=${RS/*_}
                RS=${RS/*-}
                [[ $RS =~ ^[0-9]+$ ]] && RS="SH:$RS"
                TAGS+=(`echo $RS | tr '[:lower:]' '[:upper:]'`)
            fi
        fi
    fi
fi

# Extra logging
if shopt -q login_shell && [ ${#EXTRALOG[@]} -gt 0 ]; then
    echo
    for x in "${EXTRALOG[@]}"; do
        echo -e "   *  $x"
    done
    echo
fi

# Prompts
[ "x$TERM" == "xscreen" ] && TAGS+=("\[\033[${COLOR_SCREEN}m\]screen\[\033[${COLOR_TAGS}m\]")
if [ "x${TAGS[*]}" != "x" ]; then
    PREFIX="\[\033[${COLOR_DOMAIN}m\][\[\033[${COLOR_TAGS}m\]${TAGS[@]}\[\033[${COLOR_DOMAIN}m\]] "
fi
PS1="$PREFIX\[\e[${COLOR_HOST}m\]${HOST}\[\e[${COLOR_DOMAIN}m\].${DOMAIN}\[\e[0m\]:\w\\$ "
PROMPT_COMMAND='echo -ne "\033]0;${HOST}.${DOMAIN}\007"'
MYSQL_PS1="\u@"$HOST.$DOMAIN" [\d]> "

# ITerm-specific colors
if shopt -q login_shell ; then
    echo -ne "\033]6;1;bg;red;brightness;${ITERM_COLOR[0]}\a"
    echo -ne "\033]6;1;bg;green;brightness;${ITERM_COLOR[1]}\a"
    echo -ne "\033]6;1;bg;blue;brightness;${ITERM_COLOR[2]}\a"
    function cleanup_iterm {
        echo -ne "\033]6;1;bg;*;default\a"
    }
    trap cleanup_iterm EXIT
fi

# Export it out
export PS1 MYSQL_PS1 PROMPT_COMMAND
