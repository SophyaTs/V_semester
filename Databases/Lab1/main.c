#include <stdio.h>
#include <stdlib.h>
#include <time.h>



//TODO: init db, open db, close db, correct names, add const, while for uniqueness check, case for deleting last element

const int MANAGER_SIZE = 12; // 3 long ints
const long int RECORDS_PER_UNIT = 50;
const long int MAX_DELETION = 5;
const unsigned long int Q = 114750541; //hash constant
const unsigned long int H = 4751951; //hash constant
const int FIELD_LENGTH = 20;

char im[256] = "index_main.db";
char is[256] = "index_sub.db";
char tm[256] = "table_main.db";
char ts[256] = "table_sub.db";
char mm[256] = "manager_main.db";
char ms[256] = "manager_sub.db";



typedef struct  {
    char name[20];
    char country[20];
    //char endmarker[4];
} Airline;
void airline_print(Airline* a) {
    printf("(hash = %d) %s - ", hash(a->name), a->name);
    printf("%s\n", a->country);
}

typedef struct {
    char code[20];
    char to[20];
    char from[20];
    unsigned long int main_pk;
} Flight;
void flight_print(Flight* f) {
    printf("(hash = %d) %s - ", hash(f->code), f->code);
    printf("%s - ", f->from);
    printf("%s - ", f->to);
    printf("%d\n", f->main_pk);
}

typedef struct {
    unsigned char present; // 1 - present, 0 - deleted
    unsigned long int PK;
    long int ind;
    long int aux_pos; //auxiliary index - for quicker search; for main item -> index of head subitrem list, for sub -> index to next in list
} IndexCell;

typedef struct {
    FILE* find; //file for indexes
    FILE* ftbl; //table
    FILE* fmng; //file for resource manager
    IndexCell* tind;
    long int deletion_manager;
    long int total;
    long int count;
} Table;
Table airlines, flights;




long int hash(char* str) {  
    unsigned long int hash = 0;
    for (int i = 0; (i < FIELD_LENGTH-1) && str[i]; ++i)
        hash = (H * hash + str[i]) % Q;
    return hash % Q;
}

void endl() { printf("\n"); }

void insertion_sort(long int* array, long int len) {
    int j;
    for (int i = 0; i < len; ++i) {
        j = i;
        while (j && array[j - 1] > array[j]) {
            long int swap = array[j - 1];
            array[j - 1] = array[j];
            array[j] = swap;
            j--;
        }
    }
}

long int l_binary_search(const long int* array, const long int key, const long int left, const long int right) {
    //printf("key = %d, left = %d, right = %d\n", key, left, right);
    
    if (left == right-1) {
        //printf("comapring key = %d, arr[left] = %d, arr[right] = %d\n", key, array[left], array[right]);
        if (key < array[right]) {
           // printf("choosed left \n"); endl(); endl();
            return left;
        }
        //printf("choosed right \n"); endl(); endl();
        return right;
    }

    long int m = (long int)((left + right) / 2);

    //printf("m = %d\n", m);

    if (array[m] < key)
        l_binary_search(array, key, m, right);
    else
        l_binary_search(array, key, left, m);
}

/*
 * @note: For different types of files (e.g. table and index) unit size varies. Pass 0 to bytes_per_unit to create manager file
 */
//int open_file(char* file_name, const long int bytes_per_unit) {
//    // old one!
//    int file_id = open(file_name, O_RDWR);
//    if (file_id == -1) {
//        file_id = open(file_name, O_CREAT | O_RDWR);
//        close(file_id);        
//        file_id = open(file_name, O_RDWR);
//        
//        if (bytes_per_unit != 0)
//            chsize(file_id, RECORDS_PER_UNIT*bytes_per_unit);
//        
//        else {
//            long int ma = 1;
//            chsize(file_id, (long int) MANAGER_SIZE*ma);
//            
//            //fill with starter parameters
//            long int p = -1;
//            if (lseek(file_id, 0, SEEK_SET) < 0){          
//                printf("lseek error\n");
//            }
//            write(file_id, &p, sizeof(long int));
//            p = 2;
//            lseek(file_id, 8, SEEK_SET);
//            write(file_id, &p, sizeof(long int));
//        }
//    }
//    return file_id;
//}
//
//int open_db_file(char* file_name, const long int bytes_per_unit) {
//    int file_id = open(file_name, O_RDWR);
//    if (file_id == -1) {
//        file_id = open(file_name, O_CREAT | O_RDWR);
//        //flushall();
//        if (close(file_id) == 0) {
//            file_id = open(file_name, O_RDWR);
//
//            if (bytes_per_unit != 0)
//                chsize(file_id, RECORDS_PER_UNIT * bytes_per_unit);
//        }
//        else 
//        {
//            printf("close files for %s", file_name);
//        }
//      
//    }
//    return file_id;
//}


FILE* open_db_file(const char* file_name, const long int size) {
    FILE* fd;
    //fopen_s(&fd ,file_name, "r+");
    fopen_s(&fd, file_name, "wb+");
    if (fd == NULL) {
        fopen_s(&fd, file_name, "wb+");
    }
    return fd;
}

void open_db() {
    airlines.ftbl = open_db_file(tm, sizeof(Airline));
    airlines.total = 0;
    airlines.count = 0;
    airlines.deletion_manager = 0;
    airlines.tind = (IndexCell*)malloc(RECORDS_PER_UNIT * sizeof(IndexCell));

    flights.ftbl= open_db_file(ts, sizeof(Flight));
    flights.total = 0;
    flights.count = 0;
    flights.deletion_manager = 0;
    flights.tind = (IndexCell*)malloc(RECORDS_PER_UNIT * sizeof(IndexCell));
}


void read_db(FILE* fd, unsigned char* buf, size_t size, long int offset_units) {
    fseek(fd, size * offset_units, SEEK_SET);
    fread(buf, size, 1, fd);
}

void write_db(FILE* fd, unsigned char* buf, size_t size, long int offset_units) {
    fseek(fd, size * offset_units, SEEK_SET);
    fwrite(buf, size, 1, fd);
    fflush(fd);
}

void print_tind(Table* t) {
    printf("INDEX TABLE: pos - present - PK - index - auxiliary pos\n");
    for (int i = 0; i < t->total; ++i)
        printf("%d - %d - %d - %d - %d\n", 
            i, t->tind[i].present, t->tind[i].PK, t->tind[i].ind, t->tind[i].aux_pos);
}

void print_data_m() {   
    printf("AIRLINES: DATA: index - name - country"); endl();
    for (int i = 0; i < airlines.count; ++i) {
        Airline a;       
        read_db(airlines.ftbl, &a, sizeof(Airline),i);
        
        printf("--------------------------------------------------------------------------------------------------"); endl();
        printf("%d - ", i);
        airline_print(&a);        
    }
}

void print_data_s() {
    printf("FLIGHTS: DATA: index - code - from - to - main item PK"); endl();
    for (int i = 0; i < flights.count; ++i) {
        Flight f;       
        read_db(flights.ftbl, &f, sizeof(Flight), i);
        
        printf("--------------------------------------------------------------------------------------------------"); endl();
        printf("%d - ", i);
        flight_print(&f);       
    }
}

void clean_m() {
    IndexCell* new_tind = (IndexCell*)malloc(airlines.count * sizeof(IndexCell));
    long int j = 0;
    for (long int i = 0; i < airlines.total; ++i) {
        if (airlines.tind[i].present != 0) {
            new_tind[j] = airlines.tind[i];
            ++j;
        }
    }
    free(airlines.tind);
    airlines.tind = new_tind;

    airlines.deletion_manager = 0;
    airlines.total = airlines.count;
}

void clean_s() {
    //print_tind(&flights); endl();

    IndexCell* new_tind = (IndexCell*)malloc(flights.count * sizeof(IndexCell));
    long int delete_count = flights.total - flights.count;
    long int* delete_pos = (long int*)malloc(delete_count * sizeof(long int));
    

    long int j = 0;
    long int k = 0;
    for (long int i = 0; i < flights.total; ++i) {
        if (flights.tind[i].present != 0) {
            new_tind[j] = flights.tind[i];
            ++j;
        }
        else {
            delete_pos[k] = i;
            ++k;
        }
    }
    insertion_sort(delete_pos, flights.total - flights.count);

    /*for (int i = 0; i < delete_count; ++i)
        printf("%d ", delete_pos[i]);
    endl();*/

    for (long int i = 0; i < flights.count; ++i) {
        if (new_tind[i].aux_pos != -1) {
            long int shift = l_binary_search(delete_pos, new_tind[i].aux_pos, 0, delete_count - 1);
            if (new_tind[i].aux_pos > delete_pos[0])
                ++shift;
            //printf("Shift = %d for %d\n", shift, new_tind[i].aux_pos);
            new_tind[i].aux_pos = new_tind[i].aux_pos - shift;
        }
    }
    for (long int i = 0; i < airlines.total; ++i)
        if ((airlines.tind[i].present != 0) && (airlines.tind[i].aux_pos!=-1)) {
            long int shift = l_binary_search(delete_pos, airlines.tind[i].aux_pos, 0, delete_count - 1);
            if (airlines.tind[i].aux_pos > delete_pos[0])
                ++shift;
            airlines.tind[i].aux_pos = airlines.tind[i].aux_pos - shift;
        }

    free(delete_pos);
    free(flights.tind);
    flights.tind = new_tind;

    flights.deletion_manager = 0;
    flights.total = flights.count;
}

long int add_to_sublist(long int pos_main, long int pos_sub) {
    long int next_sub = airlines.tind[pos_main].aux_pos;
    airlines.tind[pos_main].aux_pos = pos_sub;

    return next_sub;
}

void adjust_mng_info_after_insert(Table* t) {
    if (t->total == t->count)
        ++t->total;
    ++t->count;
}

void adjust_mng_info_after_removal(Table* t) {
    t->count--;
    t->deletion_manager++;
}

//TODO: check if all memory is used
long int get_insert_index(Table* t) {
    return t->count;
}

long int* get_substitute_indexes(Table* t, const long int delete_count, long int* delete_ind) {
    long int i = 0;
    long int j = 0;
    long int k = 0;

    long int* result = (long int)malloc(delete_count*sizeof(long int));
    while (i < delete_count) {
        if (t->count - 1 - k != delete_ind[delete_count - 1 - j]) {
            result[i] = t->count - 1 - k;
            ++i;
        }
        else
            ++j;
        ++k;
    }
    return result;
}

//returns first non-lesser
long int binary_search(IndexCell* tind, unsigned long int key, long int left, long int right) {
    if (left >= right) {        
        if(tind[left].PK<key)
            return left+1;
        
        return left;
    }
    
    long int m = (long int)((left + right) / 2);  
    if (tind[m].PK == key)
        return m;
    
    if (tind[m].PK < key)
        binary_search(tind, key, m+1, right);
    else
        binary_search(tind, key, left, m-1);
}

//during insertion checks for uniqueness
long int insert_to_tind(Table* t, IndexCell* cell) {
    long int pos = 0;
    if (t->total == 0) {} //it's the first item - do nothing, just insert
    else { //if there're another items we should search for appropriate position
        pos = binary_search(t->tind, cell->PK, 0, t->total - 1);
        
        //check for uniqueness   
        if ((pos < t->total)
        && ((t->tind[pos].PK == cell->PK) && (t->tind[pos].present != 0)))
        {
            return -1;
        }

        //if item's PK is in the middle, we should shift greater PKs
        if (pos < t->total) {
            for (int i = t->total; i > pos; --i)
                t->tind[i] = t->tind[i - 1];
        }
    }
    
    t->tind[pos] = *cell; 
    adjust_mng_info_after_insert(t, cell->ind);

    return pos;
}

long int search_pos_tind(Table* t, unsigned long int key) {
    long int pos = binary_search(t->tind, key, 0, t->total - 1);

    if ((pos >= t->total)
        || ((pos < t->total) && (t->tind[pos].PK != key)))
    {
        printf("Error: No such item in database\n");
        pos = -1;
    }
    return pos;
}

long int search(Table* t, unsigned long int key) {
    long int pos = search_pos_tind(t, key);
    return pos != -1? t->tind[pos].ind : -1;
}






int insert_m(char* info[]) {   
    long int ind = get_insert_index(&airlines);
    
    Airline a;
    
  /*  a.endmarker[0] = 'a';
    a.endmarker[1] = 'a';
    a.endmarker[2] = 'a';
    a.endmarker[3] = 0;*/
    
    strcpy(a.name, info[0]);
    strcpy(a.country, info[1]);
    
    unsigned long int h = hash(a.name);

    IndexCell cell;
    cell.present = 1;
    cell.PK = h;
    cell.ind = ind;
    cell.aux_pos = -1;
    
    int pos = insert_to_tind(&airlines, &cell);
    if (pos == -1) {
        printf("Error: There's an item with same PK\n");
        return -1;
    }

    write_db(airlines.ftbl, &a, sizeof(Airline), ind);

    //printf("Main item inserted %d\n", h);

    return 0;
}

int insert_s(char* info[]) {
    long int ind = get_insert_index(&flights);

    Flight f;
    strcpy(f.code, info[0]);
    strcpy(f.from, info[1]);
    strcpy(f.to, info[2]);

    unsigned long int h = hash(f.code);
    unsigned long int main_h = hash(info[3]);

    long int pos_main = search_pos_tind(&airlines, main_h);
    if (pos_main == -1)
        return -1;
    f.main_pk = main_h;
    //printf("Found main item with hash %d\n",main_h);

    IndexCell cell;
    cell.present = 1;
    cell.PK = h;
    cell.ind = ind;
    cell.aux_pos = -1;
    int pos = insert_to_tind(&flights, &cell);
    if (pos == -1) {
        printf("Error: There's an item with same PK or something else\n");
        return -1;
    }

    flights.tind[pos].aux_pos = add_to_sublist(pos_main, pos);

    write_db(flights.ftbl, &f, sizeof(Flight),ind);

    return 0;
}

void get_m(char* key) {
    long int ind = search(&airlines, hash(key));
    if (ind != -1) {
        Airline a;
        read_db(airlines.ftbl, &a, sizeof(Airline), ind);
    }
}

void get_s(char* key) {
    long int ind = search(&flights, hash(key));
    if (ind != -1) {
        Flight f;
        read_db(flights.ftbl, &f, sizeof(Flight), ind);
        flight_print(&f);
    }
}

long int count_m() {
    return airlines.count;
}

long int count_s() {
    return flights.count;
}

//TODO
long int count_s_for_m(char* key) {
    long int pos = search(&airlines, hash(key));
    if (pos == -1)
        return -1;
    
    
}

int delete_s(char* key) {
    long int pos = search_pos_tind(&flights, hash(key));
    if (pos == -1)
        return -1;

    //remove from sublist by changing aux_pos; pr=0
    Flight f;
    read_db(flights.ftbl, &f, sizeof(Flight),flights.tind[pos].ind);
    
    long int main_pos = search_pos_tind(&airlines, f.main_pk);
    long int ptr = airlines.tind[main_pos].aux_pos;
    if (pos != ptr) {
        while (flights.tind[ptr].aux_pos != pos)
            ptr = flights.tind[ptr].aux_pos;        
    }
    else {
        airlines.tind[main_pos].aux_pos = flights.tind[pos].aux_pos;
    }
    flights.tind[ptr].aux_pos = flights.tind[pos].aux_pos;
    flights.tind[pos].present = 0;    
    
    //get substitute
    long int delete_ind[] = { flights.tind[pos].ind };
    long int* substitute = get_substitute_indexes(&flights, 1, delete_ind);

    //rewrite substitute on new position
    if (delete_ind[0] < flights.count-1) {
        read_db(flights.ftbl, &f, sizeof(Flight), substitute[0]);
        write_db(flights.ftbl, &f, sizeof(Flight), delete_ind[0]);
        long int sub_pos = search_pos_tind(&flights, hash(f.code));
        flights.tind[sub_pos].ind = delete_ind[0];
    }
    
    adjust_mng_info_after_removal(&flights);
    free(substitute);

    if (flights.deletion_manager >= MAX_DELETION)
        clean_s();

    return 0;
}

int delete_m(char* key) {
    long int pos = search_pos_tind(&airlines, hash(key));
    if (pos == -1)
        return -1;
    
    //deleting main item
    airlines.tind[pos].present = 0;    
    long int delete_main[] = { airlines.tind[pos].ind };
    long int* subst_main = get_substitute_indexes(&airlines, 1, delete_main);
    if (delete_main[0] < airlines.count-1) {
        Airline a;
        read_db(airlines.ftbl, &a, sizeof(Airline), subst_main[0]);
        write_db(airlines.ftbl, &a, sizeof(Airline), delete_main[0]);
        long int subst_pos_main = search_pos_tind(&airlines, hash(a.name));
        airlines.tind[subst_pos_main].ind = delete_main[0];
    }
    adjust_mng_info_after_removal(&airlines);

    //count how many subitems
    long int sub_count = 0;
    long int ptr = airlines.tind[pos].aux_pos;
    while (ptr != -1) {
        ++sub_count;
        ptr = flights.tind[ptr].aux_pos;
    }
    
    //actually remember their indexes
    long int* delete_indexes = (long int*)malloc(sizeof(long int) * sub_count);
    ptr = airlines.tind[pos].aux_pos;
    int i = 0;
    while (ptr != -1) {
        delete_indexes[i] = flights.tind[ptr].ind;
        flights.tind[ptr].present = 0;        
        ++i;
        ptr = flights.tind[ptr].aux_pos;
    }
    insertion_sort(delete_indexes, sub_count);
    
    printf("Delete indexes: ");
    for (int i = 0; i < sub_count; ++i)
        printf("%d ", delete_indexes[i]);
    endl();

    //substitute
    long int* subst_indexes = get_substitute_indexes(&flights, sub_count, delete_indexes);

    printf("Substitute with indexes: ");
    for (int i = 0; i < sub_count; ++i)
        printf("%d ", subst_indexes[i]);
    endl();

    Flight f;
    long int final_count = flights.count - sub_count;
    for (int i = 0; i < sub_count; ++i) {
        if (delete_indexes[i] < final_count) {
            printf("(alst index will be = %d) substituting ind %d with ind %d\n", final_count-1, delete_indexes[i], subst_indexes[i]);
            read_db(flights.ftbl, &f, sizeof(Flight), subst_indexes[i]);
            write_db(flights.ftbl, &f, sizeof(Flight), delete_indexes[i]);
            long int sub_pos = search_pos_tind(&flights, hash(f.code));
            flights.tind[sub_pos].ind = delete_indexes[i];
        }
        adjust_mng_info_after_removal(&flights);
    }
    
    free(subst_main);
    free(delete_indexes);
    free(subst_indexes);

    if (airlines.deletion_manager >= MAX_DELETION)
        clean_m();
    if (flights.deletion_manager >= MAX_DELETION)
        clean_s();

    return 0;
}

void print_db() {
    printf("============================================ DATABASE ============================================\n");
    
    print_data_m();
    endl();
    print_tind(&airlines);
    endl(); endl();
    
    print_data_s();
    endl();
    print_tind(&flights);
    
    printf("==================================================================================================\n");
}



int main() {
    open_db();

    char* inf[4];
    for (int i = 0; i < 4; ++i) {
        inf[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
    }

    char* random_words[30];
    for (int i = 0; i < 10; ++i) {
        random_words[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
        sprintf_s(random_words[i], FIELD_LENGTH, "AirLine%04d" ,i);
    }

    for (int i = 10; i < 30; ++i) {
        random_words[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
        sprintf_s(random_words[i], FIELD_LENGTH, "Flight%04d", i-10);
    }

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 4; ++j)
            strcpy(inf[j], random_words[i]);

        insert_m(inf);
        
    }
    //print_data_m();
    //printf("INSERTING SUBITEMS\n"); endl(); endl();

    srand((unsigned)time(NULL));

    for (int i = 10; i < 30; ++i) {
        for (int j = 0; j < 3; ++j)
            strcpy(inf[j], random_words[i]);
                
        int k = (int)(((double)rand() / RAND_MAX) * 9);
        //printf("random number %d\n", k);
        strcpy(inf[3], random_words[k]);  

        insert_s(inf);
        //print_data_s();
        //endl(); endl();
    }

    print_db(); endl(); endl();

   
    char key[20];
    for (int i = 0; i < 5; ++i) {
        fgets(key, sizeof(key), stdin);
        key[strlen(key) - 1] = 0;

        delete_m(key);

        print_db();
        
    }

    print_db(); endl(); endl();


    fclose(flights.ftbl);
    fclose(airlines.ftbl);

    system("pause");
    return 0;
}