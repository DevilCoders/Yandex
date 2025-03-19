#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int32_t users[] = {55,283};
char votes[] = {1,5,3,6,0, 1,5,3,5,0};
char votesid[] = {30,50,10,40,0, 30,50,10,40,0 };
int voteidx[]={0,5};
int N = 2;

int8_t samevote90pct(int32_t* vcnt) {
    int32_t cnt=0;
    for(int i=0;i<10;++i) {
        cnt += vcnt[i];
    }
    for(int i=0;i<10;++i) {
        if(10*vcnt[i] > 9*cnt)
            return 1;
    }
    return 0;
}

/*
55 30 1
55 50 5
55 10 3
55 40 6
283 30 1
283 50 5
283 10 3
283 40 5
*/
void create_cf_files() {
        /* open files for future mmap */
        FILE* fusr = fopen("users.cf3","w");
        FILE* fusrdis = fopen("users_disabled.cf3","w");
        FILE* fvid = fopen("votesid.cf3","w");
        FILE* fv = fopen("votes.cf3","w");
        FILE* fvidx = fopen("voteidx.cf3","w");

        int32_t uid=0,vid=0,v=0,olduid=0,cnt=0,zero=0,ucnt=0; 
    int8_t usrdisflag=0;
    int32_t* votescnt = (int32_t*)malloc(10*sizeof(int32_t));
    memset(votescnt,0,sizeof(int32_t)*10);
        fwrite((void*)&cnt,1,sizeof(cnt),fvidx);
        while(3 == fscanf(stdin,"%d\t%d\t%d\n",&uid,&vid,&v)) {
        if(v <=0 || v > 10)
            continue;
                if( uid != olduid ) {
                        ++ucnt;
                        fwrite((void*)&uid,1,sizeof(uid),fusr);
                        if(olduid != 0) {
                                fwrite((void*)&zero,1,sizeof(int8_t),fv);
                                fwrite((void*)&zero,1,sizeof(zero),fvid);
                                ++cnt;
                                fwrite((void*)&cnt,1,sizeof(cnt),fvidx);

                // create index for disabled users ( > 90% marks same;
                usrdisflag = samevote90pct(votescnt);
                fwrite((void*)&usrdisflag,1,sizeof(int8_t),fusrdis); // enable user

                // user enabled by default
                usrdisflag=0;
                memset(votescnt,0,sizeof(int32_t)*10);
                        }
                        olduid = uid;
                }
                int8_t vote = (int8_t)v;
                fwrite((void*)&vote,1,sizeof(int8_t),fv);
                fwrite((void*)&vid,1,sizeof(vid),fvid);
                ++cnt;
        assert(vote <= 10);
        assert(vote >= 0);
        votescnt[vote-1]=votescnt[vote-1]+1;

        }

        fwrite((void*)&zero,1,sizeof(int8_t),fv);
        fwrite((void*)&zero,1,sizeof(zero),fvid);
    ++cnt;
    fwrite((void*)&cnt,1,sizeof(cnt),fvidx);

    usrdisflag = samevote90pct(votescnt);
    fwrite((void*)&usrdisflag,1,sizeof(int8_t),fusrdis); // enable user

        fclose(fusr);
        fclose(fv);
        fclose(fvid);
        fclose(fvidx);

    free(votescnt);
        printf("%d\n",ucnt);
}

int main() {
        create_cf_files();
        return 0;
}

