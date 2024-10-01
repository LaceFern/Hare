#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <string.h>

#define rect_N_MAX  9000000
#define LOCKID_OVERFLOW  0xffffffff

using namespace std;
typedef unsigned int    var_lock;
typedef unsigned int    var_freq;
typedef unsigned int    var_keySeg;

double reciPowSum(
    double para_a,
	unsigned int para_n
){
    double res = 0;
	for(unsigned int i = 0; i < para_n; i++){
		res += pow(1.0 / (i + 1), para_a);
	}
	return res;
}

var_lock genRandom_lock( 
){
	static var_lock lfsr = 1;
	var_lock lsb = lfsr & 1;
	lfsr >>= 1;
	if (lsb == 1){
		var_lock oneBit = 1;
		lfsr ^= (oneBit << 31) | (oneBit << 21) | (oneBit << 1) | (oneBit << 0);
	}
	var_lock wire_lfsr = lfsr;
	return wire_lfsr;
}

var_freq genRandom_freq(
    unsigned int lfsrSeed,
    unsigned int startFlag
){
    static var_freq lfsr = 1;
    if(startFlag == 1){
        lfsr = lfsrSeed;
    }
    else{
	    var_freq lsb = lfsr & 1;
	    lfsr >>= 1;
	    if (lsb == 1){
	    	var_freq oneBit = 1;
	    	lfsr ^= (oneBit << 31) | (oneBit << 21) | (oneBit << 1) | (oneBit << 0);
	    }
    }
    var_freq wire_lfsr = lfsr;
	return wire_lfsr;
}

int main(){
    double rect_n_arr[] = {9000000}; //lock num in dataset
    double rect_m_arr[] = {32768};
    double rect_a_arr[] = {0.99, 0.9, 0.8, 0.7, 0.6, 0.5};
    double rect_lockNumInTxn_arr[] = {1};
    double rect_txnNum_arr[] = {8388608}; // txn num in test
    double rect_lfsrSeed_arr[] = {1}; // txn num in test
    char* type_arr[] = {"hot-in"};
    double changeNum_arr[] = {256,512,1024,2048,4096,8192,16384,32768,65536};

    unsigned int rect_n_arr_len = sizeof(rect_n_arr)/sizeof(*rect_n_arr);
    unsigned int rect_m_arr_len = sizeof(rect_m_arr)/sizeof(*rect_m_arr);
    unsigned int rect_a_arr_len = sizeof(rect_a_arr)/sizeof(*rect_a_arr);
    unsigned int rect_lockNumInTxn_arr_len = sizeof(rect_lockNumInTxn_arr)/sizeof(*rect_lockNumInTxn_arr);
    unsigned int rect_txnNum_arr_len = sizeof(rect_txnNum_arr)/sizeof(*rect_txnNum_arr);
    unsigned int rect_lfsrSeed_arr_len = sizeof(rect_lfsrSeed_arr)/sizeof(*rect_lfsrSeed_arr);
    unsigned int type_arr_len = sizeof(type_arr)/sizeof(*type_arr);
    unsigned int changeNum_arr_len = sizeof(changeNum_arr)/sizeof(*changeNum_arr);

    //key loop 1
    for(unsigned int rect_n_idx = 0; rect_n_idx < rect_n_arr_len; rect_n_idx++){
        unsigned int rect_n = rect_n_arr[rect_n_idx];

        var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * rect_n);
        var_freq* freq_bs_arr = (var_freq*)malloc(sizeof(var_freq) * rect_n);

        for(unsigned int rect_m_idx = 0; rect_m_idx < rect_m_arr_len; rect_m_idx++){
            unsigned int rect_m = rect_m_arr[rect_m_idx];

            //key loop 2
            for(unsigned int rect_a_idx = 0; rect_a_idx < rect_a_arr_len; rect_a_idx++){
                double rect_a = rect_a_arr[rect_a_idx];
                //generate rect
                double reciPowSum_res = reciPowSum(rect_a, rect_N_MAX);
                printf("check: reciPowSum_res = %f\n", reciPowSum_res);
                double freqSum = 0;
                for(unsigned int i = 0; i < rect_n; i++){
                    double tmpFreq = 0;
                    if(i < rect_m){
                        tmpFreq = rect_a / rect_m;
                    }
                    else{
                        tmpFreq = (1 - rect_a) / (rect_n - rect_m);
                    }
                    freqSum += tmpFreq;
                    var_freq freq = tmpFreq * 0xFFFFFFFF;
                    freq_arr[i] = freq;
                    // printf("[%u]: freq_db = %f, freq_uint = %u\n", i, tmpFreq, freq);
                    // initiate freqCheck_arr
                }
                printf("check: freqSum(double) = %f\n", freqSum);
                //for binary search

                freq_bs_arr[0] = freq_arr[0];
                for(unsigned int i = 1; i < rect_n; i++){
                    freq_bs_arr[i] = freq_bs_arr[i - 1] + freq_arr[i];
                }
                printf("check: freqSum(uint) = %u\n", freq_bs_arr[rect_n - 1]);
                printf("info: binary research preperation ends.\n");

                //key loop 3
                for(unsigned int rect_lockNumInTxn_idx = 0; rect_lockNumInTxn_idx < rect_lockNumInTxn_arr_len; rect_lockNumInTxn_idx++){
                    unsigned int rect_lockNumInTxn = rect_lockNumInTxn_arr[rect_lockNumInTxn_idx];
                    var_lock* lockInTxn_arr = (var_freq*)calloc(sizeof(var_lock), rect_lockNumInTxn);
                    var_lock* lockInTxn_hc_arr = (var_freq*)calloc(sizeof(var_lock), rect_lockNumInTxn);

                    //key loop 4
                    for(unsigned int rect_txnNum_idx = 0; rect_txnNum_idx < rect_txnNum_arr_len; rect_txnNum_idx++){
                        unsigned int rect_txnNum = rect_txnNum_arr[rect_txnNum_idx];

                        //key loop 5
                        for(unsigned int rect_lfsrSeed_idx = 0; rect_lfsrSeed_idx < rect_lfsrSeed_arr_len; rect_lfsrSeed_idx++){
                            unsigned int rect_lfsrSeed = rect_lfsrSeed_arr[rect_lfsrSeed_idx]; 

                            var_freq* freqCheck_arr = (var_freq*)calloc(sizeof(var_freq), rect_n);
                            char resFile[128];
                            sprintf(resFile, "./res_rect/a%0.2f_n%u_lNIT%u_tN%d_sd%u.txt", \
                                    rect_a, rect_n, rect_lockNumInTxn, rect_txnNum, rect_lfsrSeed);
                            FILE *resFileOut = fopen(resFile, "w");
                            if(resFileOut == NULL) {
                                std::cout << "Error with file :" << resFile << "";
                                exit(-1);
                            }
                            printf("-----------------configs start-----------------\n");
                            printf("rect_a = %f\n rect_n = %u\n rect_lockNumInTxn = %d\n rect_txnNum = %d\n rect_lfsrSeed = %d\n", \
                                rect_a, rect_n, rect_lockNumInTxn, rect_txnNum, rect_lfsrSeed);
                            printf("-----------------configs end-----------------\n");

                            // fprintf(resFileOut, "-----------------configs start-----------------\n");
                            // fprintf(resFileOut, "rect_a = %f\n rect_n = %u\n rect_lockNumInTxn = %d\n rect_txnNum = %d\n", \
                            //     rect_a, rect_n, rect_lockNumInTxn, rect_txnNum);
                            // fprintf(resFileOut, "-----------------configs end-----------------\n");

                            //key loop 6
                            //inihot
                            genRandom_freq(rand(), 1);
                            for(unsigned int txnCount = 0; txnCount < rect_txnNum; txnCount++){ 
                            
                                //key loop 7
                                for(unsigned int lockCountInTxn = 0; lockCountInTxn < rect_lockNumInTxn;){
                                    unsigned int lock = 0;
                                    var_freq tmpFreq = genRandom_freq(rect_lfsrSeed, 0);

                                    if(txnCount < 10){
                                        printf("txnCount=%u, tmpFreq=%u\n", txnCount, tmpFreq);
                                    }

                                    // binary research
                                    if(tmpFreq <= freq_bs_arr[rect_n - 1]){
                                        if((rect_n - 1 == 0) || (tmpFreq > freq_bs_arr[rect_n - 2])){
                                            lock = rect_n - 1;
                                        }
                                        else{
                                            // search in seg 0
                                            unsigned int leftIdx = 0;
                                            unsigned int rightIdx = rect_n - 2;
	                                        do{
	                                        	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                                // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                    leftIdx, freq_bsseg0_arr[leftIdx], midIdx, freq_bsseg0_arr[midIdx], rightIdx, freq_bsseg0_arr[rightIdx], tmpFreq);

                                                if(midIdx == leftIdx){
                                                    if(tmpFreq <= freq_bs_arr[midIdx]) lock = leftIdx;
                                                    else lock = rightIdx;
                                                    break;
                                                }
                                                else{
                                                    if (tmpFreq <= freq_bs_arr[midIdx]){
                                                        if(tmpFreq > freq_bs_arr[midIdx - 1]){
                                                            lock = midIdx;
                                                            break;
                                                        }
                                                        else{
                                                            rightIdx = midIdx;
                                                        }
                                                    }
	                                        	    else{
                                                        leftIdx = midIdx;
                                                    }
                                                }
	                                        }while (leftIdx < rightIdx);
                                        }

                                    }
                                    else{
                                        lock = LOCKID_OVERFLOW;
                                    }
                                    unsigned int doubleFlag = 0;

                                    //delete overflow
                                    if(lock == LOCKID_OVERFLOW){
                                        doubleFlag = 1;
                                    }
                                    else{
                                        for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < lockCountInTxn; lockIdxInTxn++){
                                            if(lockInTxn_arr[lockIdxInTxn] == lock){
                                                doubleFlag = 1;
                                                break;
                                            }
                                        }
                                    }

                                    if(doubleFlag == 0){
                                        lockInTxn_arr[lockCountInTxn] = lock;
                                        lockCountInTxn++;
                                        //update freqCheck
                                        if(lock != LOCKID_OVERFLOW){
                                            freqCheck_arr[lock] += 1; 
                                        }

                                    }
                                }
                                //write into file!!!!!!!!!!!!!!!!!!!!!!!!
                                for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < rect_lockNumInTxn; lockIdxInTxn++){
                                    fprintf(resFileOut, "%u\t", lockInTxn_arr[lockIdxInTxn]);
                                }
                                fprintf(resFileOut, "\n");
                            }
                            fclose(resFileOut);

                            // //freqCheck
                            // string fn_freqCheckRes = "./file_freqCheckRes.txt";
                            // ofstream freqCheckRes_out;
                            // freqCheckRes_out.open(fn_freqCheckRes);
                            // for(unsigned int i = 0; i < rect_n; i++){
                            //     freqCheckRes_out << "test:" << ((double)(freqCheck_arr[i])) / (rect_txnNum * rect_lockNumInTxn) << "\t\tcal:" << (double)(freq_arr[i]) / 0xFFFFFFFF << endl;
                            // }
                            // freqCheckRes_out.close();
                            // free(freqCheck_arr);

                            //changed hot
                            for(unsigned int type_idx = 0; type_idx < type_arr_len; type_idx++){
                                for(unsigned int cNum_idx = 0; cNum_idx < changeNum_arr_len; cNum_idx++){
                                    unsigned int cNum = changeNum_arr[cNum_idx];
                                    var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * rect_n);

                                    if(strcmp(type_arr[type_idx], "hot-in") == 0){
                                        char resFile_hc[128];
                                        sprintf(resFile, "./res_rect_hc/a%0.2f_n%u_lNIT%u_tN%d_sd%u_%s_%u.txt", \
                                                rect_a, rect_n, rect_lockNumInTxn, rect_txnNum, rect_lfsrSeed, type_arr[type_idx], cNum);
                                        FILE *resFileOut_hc = fopen(resFile, "w");
                                        if(resFileOut_hc == NULL) {
                                            std::cout << "Error with file :" << resFile << "";
                                            exit(-1);
                                        }

                                        genRandom_freq(rect_lfsrSeed + 20, 1);
                                        for(unsigned int txnCount = 0; txnCount < rect_txnNum; txnCount++){ 
                                        
                                            //key loop 7
                                            for(unsigned int lockCountInTxn = 0; lockCountInTxn < rect_lockNumInTxn;){
                                                unsigned int lock = 0;
                                                var_freq tmpFreq = genRandom_freq(rect_lfsrSeed, 0);
                                                // binary research
                                                if(tmpFreq <= freq_bs_arr[rect_n - 1]){
                                                    if((rect_n - 1 == 0) || (tmpFreq > freq_bs_arr[rect_n - 2])){
                                                        lock = rect_n - 1;
                                                    }
                                                    else{
                                                        // search in seg 0
                                                        unsigned int leftIdx = 0;
                                                        unsigned int rightIdx = rect_n - 2;
	                                                    do{
	                                                    	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                                            // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                                leftIdx, freq_bsseg0_arr[leftIdx], midIdx, freq_bsseg0_arr[midIdx], rightIdx, freq_bsseg0_arr[rightIdx], tmpFreq);

                                                            if(midIdx == leftIdx){
                                                                if(tmpFreq <= freq_bs_arr[midIdx]) lock = leftIdx;
                                                                else lock = rightIdx;
                                                                break;
                                                            }
                                                            else{
                                                                if (tmpFreq <= freq_bs_arr[midIdx]){
                                                                    if(tmpFreq > freq_bs_arr[midIdx - 1]){
                                                                        lock = midIdx;
                                                                        break;
                                                                    }
                                                                    else{
                                                                        rightIdx = midIdx;
                                                                    }
                                                                }
	                                                    	    else{
                                                                    leftIdx = midIdx;
                                                                }
                                                            }
	                                                    }while (leftIdx < rightIdx);
                                                    }

                                                }
                                                else{
                                                    lock = LOCKID_OVERFLOW;
                                                }
                                                unsigned int doubleFlag = 0;

                                                //delete overflow
                                                if(lock == LOCKID_OVERFLOW){
                                                    doubleFlag = 1;
                                                }
                                                else{
                                                    for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < lockCountInTxn; lockIdxInTxn++){
                                                        if(lockInTxn_hc_arr[lockIdxInTxn] == lock){
                                                            doubleFlag = 1;
                                                            break;
                                                        }
                                                    }
                                                }

                                                if(doubleFlag == 0){
                                                    lockInTxn_hc_arr[lockCountInTxn] = lock;
                                                    lockCountInTxn++;                                                
                                                }
                                            }
                                            //write into file!!!!!!!!!!!!!!!!!!!!!!!!
                                            for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < rect_lockNumInTxn; lockIdxInTxn++){
                                                unsigned int lockId = 0;
                                                if(lockInTxn_hc_arr[lockIdxInTxn] < cNum){
                                                    lockId = rect_n - lockInTxn_hc_arr[lockIdxInTxn];
                                                }
                                                else if(lockInTxn_hc_arr[lockIdxInTxn] > rect_n - cNum){
                                                    lockId = rect_n - lockInTxn_hc_arr[lockIdxInTxn];
                                                }
                                                else{
                                                    lockId = lockInTxn_hc_arr[lockIdxInTxn];
                                                }
                                                fprintf(resFileOut, "%u\t", lockId);
                                            }
                                            fprintf(resFileOut_hc, "\n");
                                        }
                                        fclose(resFileOut_hc);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            free(freq_arr       );
            free(freq_bs_arr);
        }
    }
}