#!/bin/bash
# Переименовывает папку с графиками; создает новую и заполняет ее подпапками для хранения графиков юзеров

#mv graph_data graph_data_old
if [ "$1" == "dev" ]; then
        export graph_path=graph_dev
else
    export graph_path=/home/www/kinopoisk.ru/graph_data
fi
for dir in place_data_film variation_data_film last_vote_data_film group_vote_data_film demogr_vote_data_film career votesByYears last_comment_data last_vote_data_main last_vote_fr_data last_vote_data group_vote_data group_genre_data genre_vote_data last_fr_vote_data_main boxm popularity
do
    for x in {0..9}; do mkdir -p $graph_path/$dir/00$x/; chmod 0777 $graph_path/$dir/00$x/; done
    for x in {10..99}; do mkdir -p $graph_path/$dir/0$x/; chmod 0777 $graph_path/$dir/0$x/; done
    for x in {100..999}; do mkdir -p $graph_path/$dir/$x/; chmod 0777 $graph_path/$dir/$x/; done
    chmod 0777 $graph_path/$dir/
    echo Subdirs created: $graph_path/$dir/
done

for dir in dvd_sales survey
do
    mkdir -p $graph_path/$dir/
    chmod 0777 $graph_path/$dir/
    echo Dir created:  $graph_path/$dir/
done

mkdir -p $graph_path/boxm/weekend/
chmod 0777 $graph_path/boxm/weekend/
echo Dir created: $graph_path/boxm/weekend/

mkdir -p $graph_path/view_boxgame_user
chmod 0777 $graph_path/view_boxgame_user
