# Colorized prompt
if [ $UID != 0  ] ; then
        PS1="\[\e[32;1m\]\u@\[\e[34;1m\]\h \w \\$ \[\e[0m\]"
else
        PS1="\[\e[31;1m\]\u@\[\e[34;1m\]\h \w \\$ \[\e[0m\]"
fi

# Colorized grep
alias grep='grep --color=auto'
alias fgrep='fgrep --color=auto'
alias egrep='egrep --color=auto'
