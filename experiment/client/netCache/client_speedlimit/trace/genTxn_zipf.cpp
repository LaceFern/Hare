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

#define ZIPFPARA_N_MAX  9000000
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
    double zipfPara_n_arr[] = {1048576}; //9000000 //lock num in dataset
    double zipfPara_a_arr[] = {0.99};
    double zipfPara_lockNumInTxn_arr[] = {1};
    double zipfPara_txnNum_arr[] = {1048576}; // txn num in test
    double zipfPara_lfsrSeed_arr[] = {1}; // txn num in test
    char* type_arr[] = {"hot-in"};
    double changeNum_arr[] = {512,2048,8192,32768};

    unsigned int zipfPara_n_arr_len = sizeof(zipfPara_n_arr)/sizeof(*zipfPara_n_arr);
    unsigned int zipfPara_a_arr_len = sizeof(zipfPara_a_arr)/sizeof(*zipfPara_a_arr);
    unsigned int zipfPara_lockNumInTxn_arr_len = sizeof(zipfPara_lockNumInTxn_arr)/sizeof(*zipfPara_lockNumInTxn_arr);
    unsigned int zipfPara_txnNum_arr_len = sizeof(zipfPara_txnNum_arr)/sizeof(*zipfPara_txnNum_arr);
    unsigned int zipfPara_lfsrSeed_arr_len = sizeof(zipfPara_lfsrSeed_arr)/sizeof(*zipfPara_lfsrSeed_arr);
    unsigned int type_arr_len = sizeof(type_arr)/sizeof(*type_arr);
    unsigned int changeNum_arr_len = sizeof(changeNum_arr)/sizeof(*changeNum_arr);

    //key loop 1
    for(unsigned int zipfPara_n_idx = 0; zipfPara_n_idx < zipfPara_n_arr_len; zipfPara_n_idx++){
        unsigned int zipfPara_n = zipfPara_n_arr[zipfPara_n_idx];
        unsigned int halfParaN = zipfPara_n / 2;
        var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * zipfPara_n);
        var_freq* freq_bsseg0_arr = (var_freq*)malloc(sizeof(var_freq) * halfParaN);
        var_freq* freq_bsseg1_arr = (var_freq*)malloc(sizeof(var_freq) * (zipfPara_n - halfParaN));

        //key loop 2
        for(unsigned int zipfPara_a_idx = 0; zipfPara_a_idx < zipfPara_a_arr_len; zipfPara_a_idx++){
            double zipfPara_a = zipfPara_a_arr[zipfPara_a_idx];
            //generate zipf
            double reciPowSum_res = reciPowSum(zipfPara_a, ZIPFPARA_N_MAX);
            printf("check: reciPowSum_res = %f\n", reciPowSum_res);
            double freqSum = 0;
            for(unsigned int i = 0; i < zipfPara_n; i++){
                unsigned int x = i + 1;
                double tmpFreq = 1 / (pow(x, zipfPara_a) * reciPowSum_res); 
                freqSum += tmpFreq;
                var_freq freq = tmpFreq * 0xFFFFFFFF;
                freq_arr[i] = freq;
                // printf("[%u]: freq_db = %f, freq_uint = %u\n", i, tmpFreq, freq);
                // initiate freqCheck_arr
            }
            printf("check: freqSum(double) = %f\n", freqSum);
            //for binary search
            
            freq_bsseg0_arr[0] = freq_arr[0];
            for(unsigned int i = 1; i < halfParaN; i++){
                freq_bsseg0_arr[i] = freq_bsseg0_arr[i - 1] + freq_arr[i];
            }
            freq_bsseg1_arr[0] = freq_arr[halfParaN];
            for(unsigned int i = 1; i < zipfPara_n - halfParaN; i++){
                freq_bsseg1_arr[i] = freq_bsseg1_arr[i - 1] + freq_arr[i + halfParaN];
            }
            printf("check: freqSum(uint) = %u\n", freq_bsseg0_arr[halfParaN - 1] + freq_bsseg1_arr[zipfPara_n - halfParaN - 1]);
            printf("info: binary research preperation ends.\n");

            //key loop 3
            for(unsigned int zipfPara_lockNumInTxn_idx = 0; zipfPara_lockNumInTxn_idx < zipfPara_lockNumInTxn_arr_len; zipfPara_lockNumInTxn_idx++){
                unsigned int zipfPara_lockNumInTxn = zipfPara_lockNumInTxn_arr[zipfPara_lockNumInTxn_idx];
                var_lock* lockInTxn_arr = (var_freq*)calloc(sizeof(var_lock), zipfPara_lockNumInTxn);
                var_lock* lockInTxn_hc_arr = (var_freq*)calloc(sizeof(var_lock), zipfPara_lockNumInTxn);

                //key loop 4
                for(unsigned int zipfPara_txnNum_idx = 0; zipfPara_txnNum_idx < zipfPara_txnNum_arr_len; zipfPara_txnNum_idx++){
                    unsigned int zipfPara_txnNum = zipfPara_txnNum_arr[zipfPara_txnNum_idx];

                    //key loop 5
                    for(unsigned int zipfPara_lfsrSeed_idx = 0; zipfPara_lfsrSeed_idx < zipfPara_lfsrSeed_arr_len; zipfPara_lfsrSeed_idx++){
                        unsigned int zipfPara_lfsrSeed = zipfPara_lfsrSeed_arr[zipfPara_lfsrSeed_idx]; 

                        var_freq* freqCheck_arr = (var_freq*)calloc(sizeof(var_freq), zipfPara_n);
                        char resFile[128];
                        sprintf(resFile, "./res_zipf/a%0.2f_n%u_lNIT%u_tN%d_sd%u.txt", \
                                zipfPara_a, zipfPara_n, zipfPara_lockNumInTxn, zipfPara_txnNum, zipfPara_lfsrSeed);
                        FILE *resFileOut = fopen(resFile, "w");
                        if(resFileOut == NULL) {
                            std::cout << "Error with file :" << resFile << "";
                            exit(-1);
                        }
                        printf("-----------------configs start-----------------\n");
                        printf("zipfPara_a = %f\n zipfPara_n = %u\n zipfPara_lockNumInTxn = %d\n zipfPara_txnNum = %d\n zipfPara_lfsrSeed = %d\n", \
                            zipfPara_a, zipfPara_n, zipfPara_lockNumInTxn, zipfPara_txnNum, zipfPara_lfsrSeed);
                        printf("-----------------configs end-----------------\n");
                    
                        // fprintf(resFileOut, "-----------------configs start-----------------\n");
                        // fprintf(resFileOut, "zipfPara_a = %f\n zipfPara_n = %u\n zipfPara_lockNumInTxn = %d\n zipfPara_txnNum = %d\n", \
                        //     zipfPara_a, zipfPara_n, zipfPara_lockNumInTxn, zipfPara_txnNum);
                        // fprintf(resFileOut, "-----------------configs end-----------------\n");
                        
                        //key loop 6
                        //inihot
                        genRandom_freq(rand(), 1);
                        for(unsigned int txnCount = 0; txnCount < zipfPara_txnNum; txnCount++){ 
                        
                            //key loop 7
                            for(unsigned int lockCountInTxn = 0; lockCountInTxn < zipfPara_lockNumInTxn;){
                                unsigned int lock = 0;
                                var_freq tmpFreq = genRandom_freq(zipfPara_lfsrSeed, 0);

                                if(txnCount < 10){
                                    printf("txnCount=%u, tmpFreq=%u\n", txnCount, tmpFreq);
                                }

                                // binary research
                                if(tmpFreq <= freq_bsseg0_arr[halfParaN - 1]){
                                    if((halfParaN - 1 == 0) || (tmpFreq > freq_bsseg0_arr[halfParaN - 2])){
                                        lock = halfParaN - 1;
                                    }
                                    else{
                                        // search in seg 0
                                        unsigned int leftIdx = 0;
                                        unsigned int rightIdx = halfParaN - 2;
	                                    do{
	                                    	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                            // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                leftIdx, freq_bsseg0_arr[leftIdx], midIdx, freq_bsseg0_arr[midIdx], rightIdx, freq_bsseg0_arr[rightIdx], tmpFreq);
                        
                                            if(midIdx == leftIdx){
                                                if(tmpFreq <= freq_bsseg0_arr[midIdx]) lock = leftIdx;
                                                else lock = rightIdx;
                                                break;
                                            }
                                            else{
                                                if (tmpFreq <= freq_bsseg0_arr[midIdx]){
                                                    if(tmpFreq > freq_bsseg0_arr[midIdx - 1]){
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
                                else if(tmpFreq <= freq_bsseg0_arr[halfParaN - 1] + freq_bsseg1_arr[zipfPara_n - halfParaN - 1]){
                                    tmpFreq -= freq_bsseg0_arr[halfParaN - 1];
                                    // search in seg 1
                                    unsigned int leftIdx = 0;
                                    unsigned int rightIdx = zipfPara_n - halfParaN - 1;
                                    do{
	                                	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                        // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                            leftIdx, freq_bsseg1_arr[leftIdx], midIdx, freq_bsseg1_arr[midIdx], rightIdx, freq_bsseg1_arr[rightIdx], tmpFreq);
                                        if(midIdx == leftIdx){
                                            if(tmpFreq <= freq_bsseg1_arr[midIdx]) lock = leftIdx;
                                            else lock = rightIdx;
                                            break;
                                        }
                                        else{
                                            if (tmpFreq <= freq_bsseg1_arr[midIdx]){
                                                if(tmpFreq > freq_bsseg1_arr[midIdx - 1]){
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
                                    lock += halfParaN;
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
                            for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < zipfPara_lockNumInTxn; lockIdxInTxn++){
                                fprintf(resFileOut, "%u\t", lockInTxn_arr[lockIdxInTxn]);
                            }
                            fprintf(resFileOut, "\n");
                        }
                        fclose(resFileOut);
    
                        // //freqCheck
                        // string fn_freqCheckRes = "./file_freqCheckRes.txt";
                        // ofstream freqCheckRes_out;
                        // freqCheckRes_out.open(fn_freqCheckRes);
                        // for(unsigned int i = 0; i < zipfPara_n; i++){
                        //     freqCheckRes_out << "test:" << ((double)(freqCheck_arr[i])) / (zipfPara_txnNum * zipfPara_lockNumInTxn) << "\t\tcal:" << (double)(freq_arr[i]) / 0xFFFFFFFF << endl;
                        // }
                        // freqCheckRes_out.close();
                        // free(freqCheck_arr);
                        
                        //changed hot
                        for(unsigned int type_idx = 0; type_idx < type_arr_len; type_idx++){
                            for(unsigned int cNum_idx = 0; cNum_idx < changeNum_arr_len; cNum_idx++){
                                unsigned int cNum = changeNum_arr[cNum_idx];
                                var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * zipfPara_n);

                                if(strcmp(type_arr[type_idx], "hot-in") == 0){
                                    char resFile_hc[128];
                                    sprintf(resFile, "./res_zipf_hc/a%0.2f_n%u_lNIT%u_tN%d_sd%u_%s_%u.txt", \
                                            zipfPara_a, zipfPara_n, zipfPara_lockNumInTxn, zipfPara_txnNum, zipfPara_lfsrSeed, type_arr[type_idx], cNum);
                                    FILE *resFileOut_hc = fopen(resFile, "w");
                                    if(resFileOut_hc == NULL) {
                                        std::cout << "Error with file :" << resFile << "";
                                        exit(-1);
                                    }

                                    genRandom_freq(zipfPara_lfsrSeed + 20, 1);
                                    for(unsigned int txnCount = 0; txnCount < zipfPara_txnNum; txnCount++){ 
                                    
                                        //key loop 7
                                        for(unsigned int lockCountInTxn = 0; lockCountInTxn < zipfPara_lockNumInTxn;){
                                            unsigned int lock = 0;
                                            var_freq tmpFreq = genRandom_freq(zipfPara_lfsrSeed, 0);
                                            // binary research
                                            if(tmpFreq <= freq_bsseg0_arr[halfParaN - 1]){
                                                if((halfParaN - 1 == 0) || (tmpFreq > freq_bsseg0_arr[halfParaN - 2])){
                                                    lock = halfParaN - 1;
                                                }
                                                else{
                                                    // search in seg 0
                                                    unsigned int leftIdx = 0;
                                                    unsigned int rightIdx = halfParaN - 2;
	                                                do{
	                                                	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                                        // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                            leftIdx, freq_bsseg0_arr[leftIdx], midIdx, freq_bsseg0_arr[midIdx], rightIdx, freq_bsseg0_arr[rightIdx], tmpFreq);
                                    
                                                        if(midIdx == leftIdx){
                                                            if(tmpFreq <= freq_bsseg0_arr[midIdx]) lock = leftIdx;
                                                            else lock = rightIdx;
                                                            break;
                                                        }
                                                        else{
                                                            if (tmpFreq <= freq_bsseg0_arr[midIdx]){
                                                                if(tmpFreq > freq_bsseg0_arr[midIdx - 1]){
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
                                            else if(tmpFreq <= freq_bsseg0_arr[halfParaN - 1] + freq_bsseg1_arr[zipfPara_n - halfParaN - 1]){
                                                tmpFreq -= freq_bsseg0_arr[halfParaN - 1];
                                                // search in seg 1
                                                unsigned int leftIdx = 0;
                                                unsigned int rightIdx = zipfPara_n - halfParaN - 1;
                                                do{
	                                            	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                                    // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                        leftIdx, freq_bsseg1_arr[leftIdx], midIdx, freq_bsseg1_arr[midIdx], rightIdx, freq_bsseg1_arr[rightIdx], tmpFreq);
                                                    if(midIdx == leftIdx){
                                                        if(tmpFreq <= freq_bsseg1_arr[midIdx]) lock = leftIdx;
                                                        else lock = rightIdx;
                                                        break;
                                                    }
                                                    else{
                                                        if (tmpFreq <= freq_bsseg1_arr[midIdx]){
                                                            if(tmpFreq > freq_bsseg1_arr[midIdx - 1]){
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
                                                lock += halfParaN;
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
                                        for(unsigned int lockIdxInTxn = 0; lockIdxInTxn < zipfPara_lockNumInTxn; lockIdxInTxn++){
                                            unsigned int lockId = 0;
                                            if(lockInTxn_hc_arr[lockIdxInTxn] < cNum){
                                                lockId = zipfPara_n - lockInTxn_hc_arr[lockIdxInTxn];
                                            }
                                            else if(lockInTxn_hc_arr[lockIdxInTxn] > zipfPara_n - cNum){
                                                lockId = zipfPara_n - lockInTxn_hc_arr[lockIdxInTxn];
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
        free(freq_bsseg0_arr);
        free(freq_bsseg1_arr);
    }
}