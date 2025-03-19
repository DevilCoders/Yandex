{%- for repo  in git_local -%}
{%- for key,value in repo.iteritems() -%}
NAME={{ key }}
LOCAL_GIT_REPO=/srv/media_salt_mirror_${NAME}
GIT_DIR=/srv/git
BRANCH={{ value.branch|default("master") }}
ROBOT={{ user }}

cd /

if ! test -e $LOCAL_GIT_REPO/HEAD; then
  if test -e $LOCAL_GIT_REPO; then
    if (($(stat -c '%h' $LOCAL_GIT_REPO) > 2)); then
       mv $LOCAL_GIT_REPO ${LOCAL_GIT_REPO}_backup_$(date '+%F_%T')
    fi
  fi
  mkdir -p $LOCAL_GIT_REPO
  chown $ROBOT -R $LOCAL_GIT_REPO;
  sudo -u $ROBOT git clone --depth 1 --mirror {{ value.git }} $LOCAL_GIT_REPO
else
  if test -e $LOCAL_GIT_REPO/FETCH_HEAD; then
    chown $ROBOT $LOCAL_GIT_REPO/FETCH_HEAD;
  fi
  cd $LOCAL_GIT_REPO && sudo -u $ROBOT git remote update -p &>/dev/null
fi

if ! test -e $GIT_DIR/$NAME/.git; then
  if test -e $GIT_DIR/$NAME; then
    if (($(stat -c '%h' $GIT_DIR/$NAME) > 2)); then
       mv $GIT_DIR/$NAME $GIT_DIR/${NAME}_backup_$(date '+%F_%T')
    fi
  fi
  mkdir -p $GIT_DIR
  chown -R $ROBOT $GIT_DIR
  sudo -u $ROBOT git clone --depth 1 --branch $BRANCH \
    file://$LOCAL_GIT_REPO $GIT_DIR/$NAME
else
  cd $GIT_DIR/$NAME
  sudo -u $ROBOT git pull &>/dev/null
fi
{% endfor %}
{% endfor %}
