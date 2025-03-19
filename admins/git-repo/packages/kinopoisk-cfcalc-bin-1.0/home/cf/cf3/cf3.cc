/*
   CREATE TEMPORARY TABLE tmp_cf_proximity_cf2(
   `user_id` int(5) NOT NULL,
   `rel_user_id` int(5) NOT NULL,
   `num_all` int(5) NOT NULL,
   `proximity` float(6,2) NOT NULL,
   PRIMARY KEY (`user_id`, `rel_user_id`)
   ) ENGINE=MEMORY DEFAULT CHARSET=cp1251
   SELECT u.user_id, u2.user_id as rel_user_id, COUNT(u.id) as num_all,            
   IF (SUM(u.vote*u2.vote)-(SUM(u.vote)*SUM(u2.vote)/COUNT(u.id)) < 0, 0, ROUND(100*((SUM(u.vote*u2.vote)-(SUM(u.vote)*SUM(u2.vote)/COUNT(u.id)))/SQRT((SUM(POW(u.vote,2)) - POW(SUM(u.vote),2)/COUNT(u.id))*(SUM(POW(u2.vote,2)) - POW(SUM(u2.vote),2)/COUNT(u.id)))),2)) as proximity
   FROM urv u INNER JOIN urv u2 ON (u.id=u2.id)               
   WHERE u.user_id IN (".$user_id.") AND u2.user_id NOT IN (".$user_id.")
   GROUP BY u2.user_id
   HAVING num_all>=20 AND proximity>50
   ORDER BY null

   для каждой оценки выбирается такие же оценки, но другого пользователя.

   надо получить пользователя, его собрата, количество близких оценок и близость пользователей.

   между двумя пользователями:
   if sum(произведения оценок за каждый фильм) - sum(оценок первого)*sum(оценок второго)/count(оценок первого) < 0
   then не учитывать!
   else prox = (SUM(u.vote*u2.vote)-(SUM(u.vote)*SUM(u2.vote)/COUNT(u.id)))/SQRT((SUM(POW(u.vote,2)) - POW(SUM(u.vote),2)/COUNT(u.id))*(SUM(POW(u2.vote,2)) - POW(SUM(u2.vote),2)/COUNT(u.id)))


   sum(v*v) - sum(v)*sum(v)
   */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
/*
   int32_t users[] = {55,283};
   char votes[] = {1,5,3,6,0, 1,5,3,5,0};
   char votesid[] = {30,50,10,40,0, 30,50,10,40,0 };
   int voteidx[]={0,5};
   */
int32_t* users = NULL;
int8_t* votes = NULL;
int8_t* usrdis = NULL;
int32_t* votesid = NULL;
int32_t* voteidx = NULL;
#define VOTID(u,v) (votesid[voteidx[u] + v ]) 
#define VOT(u,v) (votes[voteidx[u] + v ]) 
#define USR(u) (users[u])

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

int32_t BinarySearch(int32_t *array, int32_t key, int32_t low, int32_t high)
{
    int32_t middle;
    if(low > high) /* termination case */
        return -1;
    middle = (low+high)/2;
    if(array[middle] == key)
        return middle;
    else if(array[middle] > key)
        return BinarySearch(array, key, low, middle-1); /* search left */
    return BinarySearch(array, key, middle+1, high); /* search right */
}

int calculate(int32_t N,int32_t workers, int32_t worker_id, const char* ids_file=NULL) {
    int fusr = open("users.cf3",O_RDONLY|O_LARGEFILE);
        int fusrdis = open("users_disabled.cf3",O_RDONLY|O_LARGEFILE);
    int fvid = open("votesid.cf3",O_RDONLY|O_LARGEFILE);
    int fv = open("votes.cf3",O_RDONLY|O_LARGEFILE);
    int fvidx = open("voteidx.cf3",O_RDONLY|O_LARGEFILE);
    FILE* fuids = ids_file?fopen(ids_file,"r"):NULL;

    if( (void*)-1 == (users = (int32_t*)mmap(NULL, lseek(fusr, 0, SEEK_END), PROT_READ, MAP_SHARED, fusr, 0)) ) {
        perror("mmap users failed: ");
        exit(1);
    }
    if( (void*)-1 == (usrdis = (int8_t*)mmap(NULL, lseek(fusrdis, 0, SEEK_END), PROT_READ, MAP_SHARED, fusrdis, 0)) ) {
        perror("mmap users failed: ");
        exit(1);
    }
    if( (void*)-1 == (voteidx = (int32_t*)mmap(NULL, lseek(fvidx, 0, SEEK_END), PROT_READ, MAP_SHARED, fvidx, 0) ) ) {
        perror("mmap voteidx failed: ");
        exit(1);
    }
    if( (void*)-1 == (votesid = (int32_t*)mmap(NULL, lseek(fvid, 0, SEEK_END), PROT_READ, MAP_SHARED, fvid, 0) )) {
        perror("mmap votesid failed: ");
        exit(1);
    }
    if( (void*)-1 == (votes = (int8_t*)mmap(NULL, lseek(fv, 0, SEEK_END), PROT_READ, MAP_SHARED, fv, 0) ) ) {
        perror("mmap votes failed: ");
        exit(1);
    }

    int32_t uid = 0;
    for(int u = worker_id; u < (fuids?N:(N-1)) ; u+=workers) {
        if(unlikely(usrdis[u])) {
            continue;
        }
        if(fuids && unlikely( USR(u) > uid )) {
            if ( 1 != fscanf(fuids,"%d\n",&uid) )
                return 0;
            u = worker_id;
        }

        if(fuids && likely( USR(u) != uid ))
            continue;

        int32_t vcnt = strlen((const char*)&VOT(u,0));
        assert(vcnt < 200000);
        for(int u2 = fuids?0:(u+1); u2 < N ; ++u2) {
            if(unlikely(usrdis[u2])) {
                continue;
            }
            if(u == u2) continue;
            int64_t sv = 0;
            int64_t sv2 = 0;
            int64_t svv2 = 0;
            int64_t svv = 0;
            int64_t sv2v2 = 0;
            int32_t cnt = 0;
            int32_t v2cnt = strlen((const char*)&VOT(u2,0));
            assert(v2cnt < 200000);

            for(int v = 0 ;v < vcnt ; ++v) {
                int v2 = BinarySearch(&VOTID(u2,0),VOTID(u,v),0,v2cnt-1);
                if(v2 != -1) {
                    ++ cnt;
                    svv2 += VOT(u,v) * VOT(u2,v2);
                    svv += VOT(u,v) * VOT(u,v);
                    sv += VOT(u,v);
                    sv2v2 += VOT(u2,v2)*VOT(u2,v2);
                    sv2 += VOT(u2,v2);
                }
            }
            if(cnt < 20 || (svv2 - sv*sv2/(float)cnt) <=0)
                continue;
            float prox = 100.0f*(fabsf (svv2 - sv*sv2/(float)cnt)
                    /
                    sqrtf(fabsf((svv - sv*sv/(float)cnt)*(sv2v2 - sv2*sv2/(float)cnt)))
                    );
            if(isinff(prox) == 1) prox=100.0f;
            else if(isinff(prox) == -1 || isnanl(prox)) prox=0.0f;
            if(prox >= 50.0f ) {
                int32_t num_up = (int32_t)(sqrtf(cnt)*prox);
                printf("%lld, %d, %d, %d, %d, %.02f\n",(long long int)(((int64_t)USR(u ))<<32)+num_up, USR(u ),num_up,USR(u2),cnt,prox);
                if(fuids == NULL) {
                    printf("%lld, %d, %d, %d, %d, %.02f\n",(long long int)(((int64_t)USR(u2))<<32)+num_up, USR(u2),num_up,USR(u ),cnt,prox);
                }
            }
        }
    }

    return 0;
}

int main(int argc,char* argv[]) {
    int opt = 0;
    int32_t N = 2;
    int32_t workers = 1;
    int32_t worker_id = 1;
    const char* ids_file = NULL;
    while ((opt = getopt(argc, argv, "u:p:c:i:")) != -1) {
        switch (opt) {
            case 'u':
                N = atoi(optarg);
                break;
            case 'p':
                workers = atoi(optarg);
                break;
            case 'c':
                worker_id = atoi(optarg);
                break;
            case 'i':
                ids_file = optarg;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s <-u number_of_users> <-p number_of_parallel_processes> <-c current_processor> [-i user_ids_file]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if( N <= 0 || workers < 1 || worker_id < 1) {
        exit(2);
    }
    if( ids_file)
        return calculate(N, workers, worker_id-1, ids_file);
    return calculate(N, workers, worker_id-1);
}

