# ~/.profile: executed by Bourne-compatible login shells.

HOST=$(hostname)

if [ "$BASH" ]; then
  if [ -f ~/.bashrc_salt ]; then
    . ~/.bashrc_salt
  # If no bashrc from salt, fallback to standart bashrc
  elif [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi

if [[ $HOST =~ "stepanar" ]]; then
   . ~/.bashrc
fi

mesg n
