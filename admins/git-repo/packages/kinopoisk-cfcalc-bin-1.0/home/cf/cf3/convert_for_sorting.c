#include <stdio.h>


int main() {
    long long unsigned int user_id,vid, vote;
     
    while(scanf("%llu\t%llu\t%llu", &user_id, &vid, &vote) == 3) {
        printf("%llu\t%llu\t%llu\t%llu\n", 10000000000*user_id+vid, user_id, vid, vote);

    }
    return 0;
}

