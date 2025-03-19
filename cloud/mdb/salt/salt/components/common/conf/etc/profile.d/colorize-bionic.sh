# Colorized prompt
if [ $UID != 0  ] ; then
        export PS1="\[\e[32;1m\]\u@\[\e[34;1m\]\h \w \\$ \[\e[0m\]"
else
        export PS1="\[\e[31;1m\]\u@\[\e[34;1m\]\h\[\033[00m\] \[\033[01;32m\]\w\[\033[00m\] # "
fi

# Colorized grep
alias grep='grep --color=auto'
export GREP_COLOR="1;32"
