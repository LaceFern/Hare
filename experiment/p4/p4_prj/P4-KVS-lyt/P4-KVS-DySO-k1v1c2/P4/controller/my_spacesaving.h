
#ifndef MY_SPACESAVING
#define MY_SPACESAVING


/****************** s p a c e - s a v i n g ******************/
#define TYPE_HEAD 0 
#define TYPE_END 1
#define TYPE_NODE 2

typedef uint32_t var_obj;
typedef uint32_t var_freq;

struct FREQNODE{
    var_obj obj;
    FREQNODE* freqNode_prev; 
    FREQNODE* freqNode_next;  
};

struct FREQBUCKET{
    var_freq freq;
    FREQBUCKET* freqBucket_prev;
    FREQBUCKET* freqBucket_next;
    FREQNODE* freqNode_head; 
    FREQNODE* freqNode_end; 
};

class Space_saving{
private:
    FREQBUCKET** freqBucket_head_arr;
    FREQBUCKET** freqBucket_end_arr;   
    FREQBUCKET* freqBucket_head;
    FREQBUCKET* freqBucket_end;
    int* freqCount_arr;
    var_obj** obj4col_arr;
    uint32_t row_num, col_num, bucket4col_num;
public: 
    /*************** basic functions ***************/
    FREQNODE* freqNode_create(FREQNODE* freqNode_prev, FREQNODE* freqNode_next, var_obj obj){
        FREQNODE* freqNode_new = (FREQNODE*)malloc(sizeof(FREQNODE));
        freqNode_new->freqNode_prev = freqNode_prev;
        freqNode_new->freqNode_next = freqNode_next;
        freqNode_new->obj = obj;
        return freqNode_new;
    }
    void freqNode_delete(FREQNODE* freqNode_p, FREQBUCKET* freqBucket_p){
        if(freqNode_p->freqNode_next != NULL){
            freqNode_p->freqNode_next->freqNode_prev = freqNode_p->freqNode_prev;
        }else{
            freqBucket_p->freqNode_end = freqNode_p->freqNode_prev;
        }
        if(freqNode_p->freqNode_prev != NULL){
            freqNode_p->freqNode_prev->freqNode_next = freqNode_p->freqNode_next;
        }else{
            freqBucket_p->freqNode_head = freqNode_p->freqNode_next;
        }
        free(freqNode_p);
    }
    void freqNode_insert(FREQNODE* freqNode_p, FREQBUCKET* freqBucket_p){
        if(freqNode_p->freqNode_next != NULL){
            freqNode_p->freqNode_next->freqNode_prev = freqNode_p;
        }else{
            freqBucket_p->freqNode_end = freqNode_p;
        }
        if(freqNode_p->freqNode_prev != NULL){
            freqNode_p->freqNode_prev->freqNode_next = freqNode_p;
        }else{
            freqBucket_p->freqNode_head = freqNode_p;
        }
    }
    FREQBUCKET* freqBucket_create(FREQBUCKET* freqBucket_prev, FREQBUCKET* freqBucket_next, var_freq freq){
        FREQBUCKET* freqBucket_new = (FREQBUCKET*)malloc(sizeof(FREQBUCKET));
        freqBucket_new->freq = freq;
        freqBucket_new->freqBucket_prev = freqBucket_prev;
        freqBucket_new->freqBucket_next = freqBucket_next;
        freqBucket_new->freqNode_head = NULL;
        freqBucket_new->freqNode_end = NULL;
        return freqBucket_new;
    }
    void freqBucket_delete(FREQBUCKET* freqBucket_p){
        if(freqBucket_p->freqBucket_next != NULL){
            freqBucket_p->freqBucket_next->freqBucket_prev = freqBucket_p->freqBucket_prev;
        }
        if(freqBucket_p->freqBucket_prev != NULL){
            freqBucket_p->freqBucket_prev->freqBucket_next = freqBucket_p->freqBucket_next;
        }
        free(freqBucket_p);
    }
    void freqBucket_insert(FREQBUCKET* freqBucket_p){
        if(freqBucket_p->freqBucket_next != NULL){
            freqBucket_p->freqBucket_next->freqBucket_prev = freqBucket_p;
        }
        if(freqBucket_p->freqBucket_prev != NULL){
            freqBucket_p->freqBucket_prev->freqBucket_next = freqBucket_p;
        }
    }
    void freqBucket_prev_check(FREQBUCKET* freqBucket_p, FREQNODE* freqNode_p){
        if(freqBucket_p->freqBucket_prev->freq == freqBucket_p->freq + 1){
            FREQNODE* freqNode_new = freqNode_create(NULL, freqBucket_p->freqBucket_prev->freqNode_head, freqNode_p->obj);
            freqNode_insert(freqNode_new, freqBucket_p->freqBucket_prev);
            freqNode_delete(freqNode_p, freqBucket_p);
        }else{
            FREQBUCKET* freqBucket_new = freqBucket_create(freqBucket_p->freqBucket_prev, freqBucket_p, freqBucket_p->freq + 1);
            FREQNODE* freqNode_new = freqNode_create(NULL, NULL, freqNode_p->obj);
            freqBucket_insert(freqBucket_new);
            freqNode_insert(freqNode_new, freqBucket_new);
            freqNode_delete(freqNode_p, freqBucket_p);
        }
        if(freqBucket_p->freqNode_head == NULL){
            freqBucket_delete(freqBucket_p);
        }
    }
    /*************** algorithm functions ***************/
	void space_saving_init(uint32_t row_num, uint32_t col_num, uint32_t bucket4col_num){
        freqBucket_head_arr = (FREQBUCKET**)malloc(sizeof(FREQBUCKET*) * row_num);
        freqBucket_end_arr = (FREQBUCKET**)malloc(sizeof(FREQBUCKET*) * row_num);
        freqCount_arr = (int*)malloc(sizeof(int) * row_num);
        obj4col_arr = (var_obj**)malloc(sizeof(var_obj*) * row_num);

        for(int i = 0; i < row_num; i++){
            freqBucket_head = (FREQBUCKET*)malloc(sizeof(FREQBUCKET));
            freqBucket_end = (FREQBUCKET*)malloc(sizeof(FREQBUCKET));
            freqBucket_head->freq = 0;
            freqBucket_head->freqBucket_prev = NULL;
            freqBucket_head->freqBucket_next = freqBucket_end;
            freqBucket_head->freqNode_head = NULL;
            freqBucket_end->freq = 0;
            freqBucket_end->freqBucket_prev = freqBucket_head;
            freqBucket_end->freqBucket_next = NULL;
            freqBucket_end->freqNode_head = NULL;
            freqBucket_head_arr[i] = freqBucket_head;
            freqBucket_end_arr[i] = freqBucket_end;
            freqCount_arr[i] = 0;

            obj4col_arr[i] = (var_obj*)malloc(sizeof(var_obj) * col_num);
            for(int j = 0; j < col_num; j++){
                obj4col_arr[i][j] = 0xffffffff;
            }
        }

        this->row_num = row_num;
        this->col_num = col_num;
        this->bucket4col_num = bucket4col_num;
	}
	uint8_t space_saving_update(uint32_t hash_index, uint32_t obj){
        int row_index = hash_index;
        freqBucket_head = freqBucket_head_arr[row_index];
        freqBucket_end = freqBucket_end_arr[row_index];

        int hitFlag = 0;
        int startFlag_bucket = 1;
        FREQBUCKET* freqBucket_next = NULL;
        FREQBUCKET* freqBucket_p = NULL;
        freqBucket_p = freqBucket_head;

        do{
            if(startFlag_bucket == 1){
                freqBucket_next = freqBucket_p->freqBucket_next;
                startFlag_bucket = 0;
            }else{
                int startFlag = 1;
                FREQNODE* freqNode_next = NULL;
                FREQNODE* freqNode_p = NULL;
                freqBucket_p = freqBucket_next;
                freqBucket_next = freqBucket_p->freqBucket_next;
                do{
                    if(startFlag == 1){
                        freqNode_p = freqBucket_p->freqNode_head;
                        freqNode_next = freqNode_p->freqNode_next;
                        startFlag = 0;
                    }else{
                        freqNode_p = freqNode_next;
                        freqNode_next = freqNode_p->freqNode_next;
                    }
                    if(freqNode_p->obj == obj){
                        freqBucket_prev_check(freqBucket_p, freqNode_p);
                        hitFlag = 1;
                    }
                }while(freqNode_next != NULL);
            }
        }while(freqBucket_next->freqBucket_next != NULL);
        if(hitFlag == 0){
            if(freqBucket_end->freqBucket_prev->freq == 1){
                FREQNODE* freqNode_new = freqNode_create(NULL, freqBucket_end->freqBucket_prev->freqNode_head, obj);
                freqNode_insert(freqNode_new, freqBucket_end->freqBucket_prev);
                if(freqCount_arr[row_index] < bucket4col_num){
                    freqCount_arr[row_index]++;
                }else{
                    freqNode_delete(freqBucket_end->freqBucket_prev->freqNode_end, freqBucket_end->freqBucket_prev);
                }
            }else{
                FREQBUCKET* freqBucket_new = freqBucket_create(freqBucket_end->freqBucket_prev, freqBucket_end, 1);
                freqBucket_insert(freqBucket_new);
                FREQNODE* freqNode_new = freqNode_create(NULL, NULL, obj);
                freqNode_insert(freqNode_new, freqBucket_new);
                freqCount_arr[row_index]++;
            }
        }

        /************************check if col changes************************/
        uint8_t col_change_flag = 0;
        uint8_t col_check_index = 0;
        uint8_t check_break_flag = 0;
        startFlag_bucket = 1;
        freqBucket_next = NULL;
        freqBucket_p = freqBucket_head;
        do{
            if(startFlag_bucket == 1){
                freqBucket_next = freqBucket_p->freqBucket_next;
                startFlag_bucket = 0;
            }else{
                int startFlag = 1;
                FREQNODE* freqNode_next = NULL;
                FREQNODE* freqNode_p = NULL;
                freqBucket_p = freqBucket_next;
                freqBucket_next = freqBucket_p->freqBucket_next;
                do{
                    if(startFlag == 1){
                        freqNode_p = freqBucket_p->freqNode_head;
                        freqNode_next = freqNode_p->freqNode_next;
                        startFlag = 0;
                    }else{
                        freqNode_p = freqNode_next;
                        freqNode_next = freqNode_p->freqNode_next;
                    }

                    if(freqNode_p->obj != obj4col_arr[row_index][col_check_index]){
                        obj4col_arr[row_index][col_check_index] = freqNode_p->obj;
                        col_change_flag = 1;
                    }
                    if(col_check_index == col_num - 1){
                        check_break_flag = 1;
                        break;
                    }
                    col_check_index++;
                }while(freqNode_next != NULL);
            }
            if(col_check_index == col_num - 1){
                check_break_flag = 1;
                break;
            }
        }while(freqBucket_next->freqBucket_next != NULL);
        return check_break_flag;
    }
	uint32_t space_saving_getcol(uint32_t hash_index, uint32_t col_idx){
        int row_index = hash_index;
        return obj4col_arr[row_index][col_idx];
    }

    /*************** algorithm functions (fresh) ***************/
    int freqBucket_integrate(FREQBUCKET* freqBucket_p){
        if(freqBucket_p->freqBucket_next == NULL || freqBucket_p->freqBucket_next->freqBucket_next == NULL){
            return 1;
        }
        else if(freqBucket_p->freq != freqBucket_p->freqBucket_next->freq){
            return 1;
        }
        else{
            //link nodes
            // printf("checkpoint 0.0\n");
            freqBucket_p->freqNode_end->freqNode_next = freqBucket_p->freqBucket_next->freqNode_head;
            // printf("checkpoint 0.1\n");
            freqBucket_p->freqBucket_next->freqNode_head->freqNode_prev = freqBucket_p->freqNode_end;
            // printf("checkpoint 0.2\n");
            freqBucket_p->freqNode_end = freqBucket_p->freqBucket_next->freqNode_end;
            // printf("checkpoint 0.3\n");
            freqBucket_delete(freqBucket_p->freqBucket_next);
            return 0;
        }
    }
    void space_saving_fresh(float factor){

        // //debug
        // printf("before fresh!\n");
        // for(int i = 0; i < 4; i++){
        //     int print_flag = 0;
        //     freqBucket_head = freqBucket_head_arr[i];
        //     FREQBUCKET* tmp_freqBucket_p = freqBucket_head;
        //     while(tmp_freqBucket_p != NULL){
        //         FREQNODE* freqNode_p = NULL; 
        //         freqNode_p = tmp_freqBucket_p->freqNode_head;
        //         if(freqNode_p != NULL){
        //             print_flag = 1;
        //             printf("[freq=%d|obj=", tmp_freqBucket_p->freq);
        //         }
        //         while(freqNode_p != NULL){
        //             printf("%d", freqNode_p->obj);
        //             freqNode_p = freqNode_p->freqNode_next;
        //             if(freqNode_p != NULL) printf(",");
        //         }
        //         if(print_flag == 1) printf("]\t");
        //         tmp_freqBucket_p = tmp_freqBucket_p->freqBucket_next;
        //     };
        //     if(print_flag == 1) printf("**row=%d**\n", i);
        // }


        // fresh freq with factor
        for(int i = 0; i < row_num; i++){
            freqBucket_head = freqBucket_head_arr[i];
            int startFlag_bucket = 1;
            FREQBUCKET* freqBucket_next = NULL;
            FREQBUCKET* freqBucket_p = NULL;
            freqBucket_p = freqBucket_head;
            do{
                if(startFlag_bucket == 1){
                    freqBucket_next = freqBucket_p->freqBucket_next;
                    startFlag_bucket = 0;
                }else{
                    freqBucket_p = freqBucket_next;
                    freqBucket_p->freq = freqBucket_p->freq * factor;
                    freqBucket_next = freqBucket_p->freqBucket_next;
                }
            }while(freqBucket_next->freqBucket_next != NULL);
        }

        // printf("checkpoint 0\n");
        //integrate buckets with same freqs
        for(int i = 0; i < row_num; i++){
            freqBucket_head = freqBucket_head_arr[i];
            int startFlag_bucket = 1;
            FREQBUCKET* freqBucket_next = NULL;
            FREQBUCKET* freqBucket_p = NULL;
            freqBucket_p = freqBucket_head;
            do{
                if(startFlag_bucket == 1){
                    freqBucket_next = freqBucket_p->freqBucket_next;
                    startFlag_bucket = 0;
                }else{
                    freqBucket_p = freqBucket_next;
                    int integrate_end = 0;
                    do{
                        integrate_end = freqBucket_integrate(freqBucket_p);
                    }while(!integrate_end);
                    freqBucket_next = freqBucket_p->freqBucket_next;
                }
            }while(freqBucket_next != NULL && freqBucket_next->freqBucket_next != NULL);
        }

        // printf("checkpoint 1\n");
        //del empty buckets
        for(int i = 0; i < row_num; i++){
            freqBucket_head = freqBucket_head_arr[i];
            int startFlag_bucket = 1;
            FREQBUCKET* freqBucket_next = NULL;
            FREQBUCKET* freqBucket_p = NULL;
            freqBucket_p = freqBucket_head;
            do{
                if(startFlag_bucket == 1){
                    freqBucket_next = freqBucket_p->freqBucket_next;
                    startFlag_bucket = 0;
                }else{
                    freqBucket_p = freqBucket_next;
                    freqBucket_next = freqBucket_p->freqBucket_next;
                    if(freqBucket_p->freq == 0){
                        while(freqBucket_p->freqNode_head != NULL){
                            freqNode_delete(freqBucket_p->freqNode_head, freqBucket_p);
                            freqCount_arr[i]--;
                        }
                        freqBucket_delete(freqBucket_p);
                    }
                }
            }while(freqBucket_next->freqBucket_next != NULL);
        }

        // //debug
        // printf("after fresh!\n");
        // for(int i = 0; i < 4; i++){
        //     int print_flag = 0;
        //     freqBucket_head = freqBucket_head_arr[i];
        //     FREQBUCKET* tmp_freqBucket_p = freqBucket_head;
        //     while(tmp_freqBucket_p != NULL){
        //         FREQNODE* freqNode_p = NULL; 
        //         freqNode_p = tmp_freqBucket_p->freqNode_head;
        //         if(freqNode_p != NULL){
        //             print_flag = 1;
        //             printf("[freq=%d|obj=", tmp_freqBucket_p->freq);
        //         }
        //         while(freqNode_p != NULL){
        //             printf("%d", freqNode_p->obj);
        //             freqNode_p = freqNode_p->freqNode_next;
        //             if(freqNode_p != NULL) printf(",");
        //         }
        //         if(print_flag == 1) printf("]\t");
        //         tmp_freqBucket_p = tmp_freqBucket_p->freqBucket_next;
        //     };
        //     if(print_flag == 1) printf("**row=%d**\n", i);
        // }
        // printf("checkpoint 3\n");

    }
};

#endif