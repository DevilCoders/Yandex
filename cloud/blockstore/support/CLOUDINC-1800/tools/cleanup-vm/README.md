# Run
ya make -rA cleanup-vm/ && \
./cleanup-vm/cleanup-vm \
    --cluster hw-nbs-stable-lab \
    --ycp-profile hw-nbs-stable-lab \
    --tasks ./tasks.txt -vvv \
    --no-auth \
    --xfsprogs-path xfsprogs-5.3.0 \
    --e2fsprogs-path e2fsprogs-1.46.4 \

# tasks.txt
$ cat tasks.txt 
fooooooooobaaarbaaz1 nrd01 nrd11 nrd12
fooooooooobaaarbaaz2 nrd02 nrd12 nrd22
^                    ^    ^    ^
instance_id            disk_id
