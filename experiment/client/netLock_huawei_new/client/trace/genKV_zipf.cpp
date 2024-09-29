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
typedef unsigned int    var_key;
typedef unsigned int    var_freq;

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

var_key genRandom_key( 
){
	static var_key lfsr = 1;
	var_key lsb = lfsr & 1;
	lfsr >>= 1;
	if (lsb == 1){
		var_key oneBit = 1;
		lfsr ^= (oneBit << 31) | (oneBit << 21) | (oneBit << 1) | (oneBit << 0);
	}
	var_key wire_lfsr = lfsr;
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
    double zipfPara_n_arr[] = {1048576}; //9000000 //key num in dataset
    double zipfPara_a_arr[] = {0.99};
    double zipfPara_reqNum_arr[] = {1048576}; // req num in test
    double zipfPara_lfsrSeed_arr[] = {1}; // req num in test
    int type_arr[] = {1};
    double changeNum_arr[] = {500,1000,2000,4000,6000,8000,10000};

    unsigned int zipfPara_n_arr_len = sizeof(zipfPara_n_arr)/sizeof(*zipfPara_n_arr);
    unsigned int zipfPara_a_arr_len = sizeof(zipfPara_a_arr)/sizeof(*zipfPara_a_arr);
    unsigned int zipfPara_reqNum_arr_len = sizeof(zipfPara_reqNum_arr)/sizeof(*zipfPara_reqNum_arr);
    unsigned int zipfPara_lfsrSeed_arr_len = sizeof(zipfPara_lfsrSeed_arr)/sizeof(*zipfPara_lfsrSeed_arr);
    unsigned int type_arr_len = sizeof(type_arr)/sizeof(*type_arr);
    unsigned int changeNum_arr_len = sizeof(changeNum_arr)/sizeof(*changeNum_arr);

    //key loop 1
    for(unsigned int zipfPara_n_idx = 0; zipfPara_n_idx < zipfPara_n_arr_len; zipfPara_n_idx++){
        unsigned int zipfPara_n = zipfPara_n_arr[zipfPara_n_idx];
        var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * zipfPara_n);
        var_freq* freq_sum_arr = (var_freq*)malloc(sizeof(var_freq) * zipfPara_n);

        //key loop 2
        for(unsigned int zipfPara_a_idx = 0; zipfPara_a_idx < zipfPara_a_arr_len; zipfPara_a_idx++){
            double zipfPara_a = zipfPara_a_arr[zipfPara_a_idx];
            //generate zipf
            double reciPowSum_res = reciPowSum(zipfPara_a, zipfPara_n);
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
            
            freq_sum_arr[0] = freq_arr[0];
            for(unsigned int i = 1; i < zipfPara_n; i++){
                freq_sum_arr[i] = freq_sum_arr[i - 1] + freq_arr[i];
            }
            printf("check: freqSum(uint) = %u\n", freq_sum_arr[zipfPara_n - 1]);
            printf("info: binary research preperation ends.\n");


                //key loop 4
                for(unsigned int zipfPara_reqNum_idx = 0; zipfPara_reqNum_idx < zipfPara_reqNum_arr_len; zipfPara_reqNum_idx++){
                    unsigned int zipfPara_reqNum = zipfPara_reqNum_arr[zipfPara_reqNum_idx];

                    //key loop 5
                    for(unsigned int zipfPara_lfsrSeed_idx = 0; zipfPara_lfsrSeed_idx < zipfPara_lfsrSeed_arr_len; zipfPara_lfsrSeed_idx++){
                        unsigned int zipfPara_lfsrSeed = zipfPara_lfsrSeed_arr[zipfPara_lfsrSeed_idx]; 

                        var_freq* freqCheck_arr = (var_freq*)calloc(sizeof(var_freq), zipfPara_n);
                        char resFile[128];
                        sprintf(resFile, "./res_zipf/a%0.2f_n%u_rN%d_sd%u.txt", \
                                zipfPara_a, zipfPara_n, zipfPara_reqNum, zipfPara_lfsrSeed);
                        FILE *resFileOut = fopen(resFile, "w");
                        if(resFileOut == NULL) {
                            std::cout << "Error with file :" << resFile << "";
                            exit(-1);
                        }
                        printf("-----------------configs start-----------------\n");
                        printf("zipfPara_a = %f\n zipfPara_n = %u\n zipfPara_reqNum = %d\n zipfPara_lfsrSeed = %d\n", \
                            zipfPara_a, zipfPara_n, zipfPara_reqNum, zipfPara_lfsrSeed);
                        printf("-----------------configs end-----------------\n");
                    
                        // fprintf(resFileOut, "-----------------configs start-----------------\n");
                        // fprintf(resFileOut, "zipfPara_a = %f\n zipfPara_n = %u\n zipfPara_reqNum = %d\n", \
                        //     zipfPara_a, zipfPara_n, zipfPara_reqNum);
                        // fprintf(resFileOut, "-----------------configs end-----------------\n");
                        
                        //key loop 6
                        //inihot
                        genRandom_freq(rand(), 1);
                        for(unsigned int reqCount = 0; reqCount < zipfPara_reqNum; reqCount++){ 
                        
                            //key loop 7
                            int validFlag = 0;
                            var_key key = 0;
                            while(validFlag == 0){
                                
                                var_freq tmpFreq = genRandom_freq(zipfPara_lfsrSeed, 0);

                                if(reqCount < 10){
                                    printf("reqCount=%u, tmpFreq=%u\n", reqCount, tmpFreq);
                                }

                                // binary research
                                if(tmpFreq <= freq_sum_arr[zipfPara_n - 1]){
                                    if((zipfPara_n - 1 == 0) || (tmpFreq > freq_sum_arr[zipfPara_n - 2])){
                                        key = zipfPara_n - 1;
                                    }
                                    else{
                                        // search in seg 0
                                        unsigned int leftIdx = 0;
                                        unsigned int rightIdx = zipfPara_n - 2;
	                                    do{
	                                    	unsigned int midIdx = (leftIdx + rightIdx) / 2;
                                            // if(throughputCount_largeScale == 11 && throughputCount == 654) printf("debug: freq[%d] = %u, freq[%d] = %u, freq[%d] = %u, tmpFreq = %u\n", \
                                                leftIdx, freq_sum_arr[leftIdx], midIdx, freq_sum_arr[midIdx], rightIdx, freq_sum_arr[rightIdx], tmpFreq);
                        
                                            if(midIdx == leftIdx){
                                                if(tmpFreq <= freq_sum_arr[midIdx]) key = leftIdx;
                                                else key = rightIdx;
                                                break;
                                            }
                                            else{
                                                if (tmpFreq <= freq_sum_arr[midIdx]){
                                                    if(tmpFreq > freq_sum_arr[midIdx - 1]){
                                                        key = midIdx;
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
                                    key = LOCKID_OVERFLOW;
                                }
                                //delete overflow
                                if(key == LOCKID_OVERFLOW){
                                    validFlag = 0;
                                }
                                else{
                                    validFlag = 1;
                                    //update freqCheck
                                    freqCheck_arr[key] += 1; 
                                }
                            }
                            //write into file!!!!!!!!!!!!!!!!!!!!!!!!
                            fprintf(resFileOut, "%u\t", key);
                            fprintf(resFileOut, "\n");
                        }
                        fclose(resFileOut);
    
                        // //freqCheck
                        // string fn_freqCheckRes = "./file_freqCheckRes.txt";
                        // ofstream freqCheckRes_out;
                        // freqCheckRes_out.open(fn_freqCheckRes);
                        // for(unsigned int i = 0; i < zipfPara_n; i++){
                        //     freqCheckRes_out << "test:" << ((double)(freqCheck_arr[i])) / (zipfPara_reqNum * zipfPara_keyNumInReq) << "\t\tcal:" << (double)(freq_arr[i]) / 0xFFFFFFFF << endl;
                        // }
                        // freqCheckRes_out.close();
                        // free(freqCheck_arr);
                        
                        //changed hot
                        for(unsigned int type_idx = 0; type_idx < type_arr_len; type_idx++){
                            for(unsigned int cNum_idx = 0; cNum_idx < changeNum_arr_len; cNum_idx++){
                                unsigned int cNum = changeNum_arr[cNum_idx];
                                var_freq* freq_arr = (var_freq*)malloc(sizeof(var_freq) * zipfPara_n);

                                    printf("-----------------configs start-----------------\n");
                                    printf("hc_type = %d\n hc_num = %u\n", \
                                        type_arr[type_idx], cNum);
                                    printf("-----------------configs end-----------------\n");

                                    char resFile_hc[128];
                                    sprintf(resFile_hc, "./res_zipf_hc/a%0.2f_n%u_rN%d_sd%u_hcT%d_hcN%u.txt", \
                                            zipfPara_a, zipfPara_n, zipfPara_reqNum, zipfPara_lfsrSeed, type_arr[type_idx], cNum);
                                    FILE *resFileOut_hc = fopen(resFile_hc, "w");
                                    if(resFileOut_hc == NULL) {
                                        std::cout << "Error with file :" << resFile_hc << "";
                                        exit(-1);
                                    }

                                    char resFile[128];
                                    sprintf(resFile, "./res_zipf/a%0.2f_n%u_rN%d_sd%u.txt", \
                                            zipfPara_a, zipfPara_n, zipfPara_reqNum, zipfPara_lfsrSeed);
                                    FILE *resFileIn = fopen(resFile, "r");
                                    if(resFileIn == NULL) {
                                        std::cout << "Error with file :" << resFile << "";
                                        exit(-1);
                                    }
                                    
                                    for(int i = 0; i < zipfPara_reqNum; i++){
                                        var_key tmpKey = 0;
                                        fscanf(resFileIn,"%u", &tmpKey);
                                        
                                        if(type_arr[type_idx] == 1){
                                            if(tmpKey < cNum){
                                                tmpKey = zipfPara_n - 1 - tmpKey;
                                            }
                                            else{
                                                tmpKey = tmpKey - cNum;
                                            }
                                        }
                                        else if(type_arr[type_idx] == 2){
                                            if(tmpKey >= zipfPara_n - cNum){
                                                tmpKey = zipfPara_n - 1 - tmpKey;
                                            }
                                            else{
                                                tmpKey = tmpKey + cNum;
                                            }
                                        }
                                        else if(type_arr[type_idx] == 3){
                                            if(tmpKey < cNum * 3){
                                                if(tmpKey % 3 == 0){
                                                    tmpKey = zipfPara_n - 1 - tmpKey;
                                                }
                                            }
                                            else if(tmpKey >= zipfPara_n - cNum * 3){
                                                if((zipfPara_n - tmpKey) % 3 == 0){
                                                    tmpKey = zipfPara_n - 1 - tmpKey;
                                                }
                                            }
                                        }
    
                                        fprintf(resFileOut_hc, "%u\t\n", tmpKey);
                                    }
                                    fclose(resFileOut_hc);
                                    fclose(resFileIn);

                            }
                        }
                    }
                }
        }
        free(freq_arr       );
        free(freq_sum_arr);
    }
}