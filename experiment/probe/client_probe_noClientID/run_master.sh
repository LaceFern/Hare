# np_arr=(1)
# host_arr=("r1")
# data_arr=("/home/tt/experiment/dataset/rcv1_train")
# size_arr=("20242 47236")
# batch_arr=(1024)
# thread_arr=(1)

# g++ -o communicator -mavx512f -mavx512f -mavx512cd -mavx512er -mavx512pf -mavx512vl -mavx512dq -mavx512bw -pthread my_communicator_final.cpp ./timer.cpp -O3 -L /usr/local/mpich-3.4.1/lib/ -lmpi

# for ((i=8; i<=10; i++))
# do
#     scp "/home/tt/experiment/cpu-training/communicator" tt@192.168.189.$i:"/home/tt/experiment/cpu-training/"
# done

make

n_arr=(4096)
a_arr=(99 95 90 80 70 60 50)
k_arr=(1 2 4 8)
t_arr=(16384)
s_arr=(1)

for i1 in "${!n_arr[@]}"
do
    for i2 in "${!a_arr[@]}"
    do
        for i3 in "${!k_arr[@]}"
        do
            for i4 in "${!t_arr[@]}"
            do
                for i5 in "${!s_arr[@]}"
                do
                    sudo ./build/netlock_client -l 0-11,24-30  -- -n${n_arr[$i1]} -a${a_arr[$i2]} -k${k_arr[$i3]} -t${t_arr[$i4]} -s${s_arr[$i5]} > "./testRes/n${n_arr[$i1]}_a${a_arr[$i2]}_k${k_arr[$i3]}_t${t_arr[$i4]}_s${s_arr[$i5]}.txt"
                done
            done
        done
    done
done
