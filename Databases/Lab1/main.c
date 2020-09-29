#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>


//TODO: correct names

const long int RECORDS_PER_UNIT = 50;
const long int MAX_DELETION = 10;
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
} Airline;
void airline_print(const Airline* a) {
    printf("(hash = %d) %s - ", hash(a->name), a->name);
    printf("%s\n", a->country);
}

typedef struct {
    char code[20];
    char to[20];
    char from[20];
    unsigned long int main_pk;
} Flight;
void flight_print(const Flight* f) {
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
    FILE* ftbl; //table
    IndexCell* tind;
    long int deletion_manager;
    long int capacity;
    long int total;
    long int count;
} Table;
Table airlines, flights;




long int hash(const char* str) {  
    unsigned long int hash = 0;
    for (int i = 0; (i < FIELD_LENGTH-1) && str[i]; ++i)
        hash = (H * hash + str[i]) % Q;
    return hash % Q;
}

void endl() { printf("\n"); }

void insertion_sort(long int* array, const long int len) {
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


void read_db(FILE* fd, unsigned char* buf, const size_t size, const long int offset_records) {
    fseek(fd, size * offset_records, SEEK_SET);
    fread(buf, size, 1, fd);
}

void write_db(FILE* fd, unsigned char* buf, const size_t size, const long int offset_records) {
    fseek(fd, size * offset_records, SEEK_SET);
    fwrite(buf, size, 1, fd);
    fflush(fd);
}

void substitute_db(FILE* fd, unsigned char* buf, const size_t size, const long int where_offset_records, const long int from_offset_records) {
    read_db(fd, &buf, size, from_offset_records);
    write_db(fd, &buf, size, where_offset_records);
}


void clean_m() {
    IndexCell* new_tind = (IndexCell*)malloc(airlines.capacity * sizeof(IndexCell));
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

    IndexCell* new_tind = (IndexCell*)malloc(flights.capacity * sizeof(IndexCell));
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

FILE* open_db_data(const char* file_name, const long int size) {
    FILE* fd;
    fopen_s(&fd, file_name, "rb+");
    if (fd == NULL) {
        fopen_s(&fd, file_name, "wb+");
        _chsize_s(_fileno(fd), size * RECORDS_PER_UNIT);
    }
    return fd;
}

void open_mng_info(const char* file_name, Table* t) {
    FILE* fd;
    fopen_s(&fd, file_name, "rb");
    if (fd == NULL) {
        t->count = t->total = t->deletion_manager = 0;
        t->capacity = RECORDS_PER_UNIT;
    }
    else {
        read_db(fd, &t->count, sizeof(long int), 0);
        read_db(fd, &t->deletion_manager, sizeof(long int), 1);
        t->total = t->count;

        t->capacity = ((long int)(t->total / RECORDS_PER_UNIT) + 1) * RECORDS_PER_UNIT;

        fclose(fd);
    }
}

void open_db_find(const char* file_name, Table* t) {
    t->tind = (IndexCell*)malloc(t->capacity * sizeof(IndexCell));

    FILE* fd;
    fopen_s(&fd, file_name, "rb");
    if (fd != NULL) {
        for (long int i = 0; i < t->total; ++i)
            read_db(fd, &t->tind[i], sizeof(IndexCell), i);

        fclose(fd);
    }
}

void open_db() {
    airlines.ftbl = open_db_data(tm, sizeof(Airline));
    open_mng_info(mm, &airlines);
    open_db_find(im, &airlines);

    flights.ftbl= open_db_data(ts, sizeof(Flight));
    open_mng_info(ms, &flights);
    open_db_find(is, &flights);
}

void remember_mng_info(const char* file_name, Table* t) {
    FILE* fd;
    fopen_s(&fd, file_name, "wb+");

    write_db(fd, &t->count, sizeof(long int), 0);
    write_db(fd, &t->deletion_manager, sizeof(long int), 1);

    fclose(fd);
}

void remember_tind(const char* file_name, Table* t) {
    FILE* fd;
    fopen_s(&fd, file_name, "wb+");
    if (fd != NULL) {
        for (long int i = 0; i < t->total; ++i)
            write_db(fd, &t->tind[i], sizeof(IndexCell), i); 

        fclose(fd);
        free(t->tind);
    }   
}

void close_db() {
    if (airlines.count < airlines.total)
        clean_m();
    if (flights.count < flights.total)
        clean_s();

    remember_mng_info(mm, &airlines);
    remember_tind(im, &airlines);
    fclose(airlines.ftbl);

    remember_mng_info(ms, &flights);
    remember_tind(is, &flights);
    fclose(flights.ftbl);
}

void change_capacity(Table* t) {
    t->capacity += RECORDS_PER_UNIT;
    IndexCell* new_tind = (IndexCell*)malloc(t->capacity * sizeof(IndexCell));
    memcpy(new_tind, t->tind, t->total * sizeof(IndexCell));

    free(t->tind);
    t->tind = new_tind;

    fflush(t->ftbl);

    size_t size = t == &airlines ? sizeof(Airline) : sizeof(Flight);
    _chsize_s(_fileno(t->ftbl), size * t->capacity);
}




void print_tind(const Table* t) {
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



long int add_to_sublist(const long int pos_main, const long int pos_sub) {
    long int next_sub = airlines.tind[pos_main].aux_pos;
    airlines.tind[pos_main].aux_pos = pos_sub;

    return next_sub;
}

void shift(const long int pos) {
    for (int i = 0; i < airlines.total; ++i)
        if ((airlines.tind[i].present == 1) && (airlines.tind[i].aux_pos >= pos))
            ++airlines.tind[i].aux_pos;

    for(int i=0;i<flights.total;++i)
        if ((flights.tind[i].present == 1) && (flights.tind[i].aux_pos >= pos))
            ++flights.tind[i].aux_pos;
}

void adjust_mng_info_after_insert(Table* t) {
    if (t->total == t->count)
        ++t->total;
    ++t->count;

    if (t->total >= t->capacity)
        change_capacity(t);
}

void adjust_mng_info_after_removal(Table* t) {
    t->count--;
    t->deletion_manager++;
}

long int get_insert_index(const Table* t) {
    return t->count;
}

long int* get_substitute_indexes(const Table* t, const long int delete_count, long int* delete_ind) {
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
long int binary_search(const IndexCell* tind, const unsigned long int key, const long int left, const long int right) {
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
long int insert_to_tind(Table* t, const IndexCell* cell) {
    long int pos = 0;
    if (t->total == 0) {} //it's the first item - do nothing, just insert
    else { //if there're other items, we should search for appropriate position
        pos = binary_search(t->tind, cell->PK, 0, t->total - 1);
        
        //check for uniqueness   
        if ((pos < t->total)
        && (t->tind[pos].PK == cell->PK))
        {
            long int ptr = pos;
            
            //maybe there are still a couple of deleted items with same PK
            while ((ptr < t->tind) 
                && (t->tind[ptr].PK == cell->PK) && (t->tind[ptr].present == 0)) 
                ++ptr;
            
            if((ptr < t->tind) && (t->tind[ptr].PK == cell->PK) && (t->tind[ptr].present != 0))
                return -1;
        }

        //if item's PK is in the middle, we should shift greater PKs
        if (pos < t->total) {
            if (t == &flights)
                shift(pos);
            for (int i = t->total; i > pos; --i)
                t->tind[i] = t->tind[i - 1];
        }
    }
    
    t->tind[pos] = *cell; 
    adjust_mng_info_after_insert(t, cell->ind);

    return pos;
}

long int search_pos_tind(const Table* t, const unsigned long int key) {
    long int pos = binary_search(t->tind, key, 0, t->total - 1);

    if ((pos >= t->total)
        || ((pos < t->total) && (t->tind[pos].PK != key)))
    {
        printf("Error: No such item in database\n");
        pos = -1;
    }
    return pos;
}

long int search(const Table* t, const unsigned long int key) {
    long int pos = search_pos_tind(t, key);
    return pos != -1? t->tind[pos].ind : -1;
}

long int count_subitems(const long int pos) {
    long int sub_count = 0;
    long int ptr = airlines.tind[pos].aux_pos;
    while (ptr != -1) {
        ++sub_count;
        ptr = flights.tind[ptr].aux_pos;
    }

    return sub_count;
}




int insert_m(const char* info[]) {
    long int ind = get_insert_index(&airlines);
    
    Airline a;   
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

    return 0;
}

int insert_s(const char* info[]) {
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
        printf("Error: There's an item with same PK or main item was not found\n");
        return -1;
    }

    flights.tind[pos].aux_pos = add_to_sublist(pos_main, pos);

    write_db(flights.ftbl, &f, sizeof(Flight),ind);

    return 0;
}

void get_m(const char* key) {
    long int ind = search(&airlines, hash(key));
    if (ind != -1) {
        Airline a;
        read_db(airlines.ftbl, &a, sizeof(Airline), ind);
        airline_print(&a);
    }
}

void get_s(const char* key) {
    long int ind = search(&flights, hash(key));
    if (ind != -1) {
        Flight f;
        read_db(flights.ftbl, &f, sizeof(Flight), ind);
        flight_print(&f);
    }
}

//edits only secondary fields, can't change PK or relation
int edit_m(const char* key, const char* new_info[]) {
    long int ind = search(&airlines, hash(key));
    Airline a;
    read_db(airlines.ftbl, &a, sizeof(Airline), ind);

    strcpy(a.country, new_info[0]);

    write_db(airlines.ftbl, &a, sizeof(Airline), ind);
}

//edits only secondary fields, can't change PK or relation
int edit_s(const char* key, const char* new_info[]) {
    long int ind = search(&flights, hash(key));
    Flight f;
    read_db(flights.ftbl, &f, sizeof(Flight), ind);

    strcpy(f.from, new_info[0]);
    strcpy(f.to, new_info[1]);

    write_db(flights.ftbl, &f, sizeof(Flight), ind);
}

long int count_s_for_m(const char* key) {
    long int pos = search_pos_tind(&airlines, hash(key));
    
    if (pos != -1) {
        return count_subitems(pos);
    }
    
    return -1;
}

int delete_s(char* key) {
    long int pos = search_pos_tind(&flights, hash(key));
    if (pos == -1)
        return -1;

    //remove from sublist by changing aux_pos; pr=0
    Flight f;
    read_db(flights.ftbl, &f, sizeof(Flight), flights.tind[pos].ind);

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
    if (delete_ind[0] < flights.count - 1) {
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
    if (delete_main[0] < airlines.count - 1) {
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

    /*printf("Delete indexes: ");
    for (int i = 0; i < sub_count; ++i)
        printf("%d ", delete_indexes[i]);
    endl();*/

    //substitute
    long int* subst_indexes = get_substitute_indexes(&flights, sub_count, delete_indexes);

   /* printf("Substitute with indexes: ");
    for (int i = 0; i < sub_count; ++i)
        printf("%d ", subst_indexes[i]);
    endl();*/

    Flight f;
    long int final_count = flights.count - sub_count;
    for (int i = 0; i < sub_count; ++i) {
        if (delete_indexes[i] < final_count) {
            //printf("(alst index will be = %d) substituting ind %d with ind %d\n", final_count - 1, delete_indexes[i], subst_indexes[i]);
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
    printf("\n============================================ DATABASE ============================================\n");
    
    print_data_m();
    endl();
    print_tind(&airlines);
    endl(); endl();
    
    print_data_s();
    endl();
    print_tind(&flights);
    
    printf("==================================================================================================\n\n");
}



void interact() {
    print_db();

    char* info[4];
    for (int i = 0; i < 4; ++i) {
        info[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
    }
    char* key = (char*)malloc(FIELD_LENGTH * sizeof(char));

    int choice = 1;
    while ((choice > 0) && (choice<14)) {
        printf("Choose action:\n 1 = Get-m\n 2 = Get-s\n 3 = Insert-m\n 4 = Insert-s\n 5 = Edit-m\n 6 = Edit-s\n 7 = Delete-m\n 8 = Delete-s\n 9 = print database\n 10 = count all records\n 11 = count main records\n 12 = count subrecords\n 13 = count subrecords for main record\n 0 = EXIT\n");
        scanf_s("%d", &choice);
        getchar();

        if (choice == 1) { //Get-m
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            get_m(key);
        } 
        else if (choice == 2) { //Get-s
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            get_s(key);
        }
        else if (choice == 3) { //Insert-m
            printf("Enter the name(PK):\n");
            fgets(info[0], FIELD_LENGTH, stdin);
            info[0][strlen(info[0]) - 1] = 0;

            printf("Enter the country:\n");
            fgets(info[1], FIELD_LENGTH, stdin);
            info[1][strlen(info[1]) - 1] = 0;

            insert_m(info);
        }
        else if (choice == 4) { //Insert-s
            printf("Enter the code(PK):\n");
            fgets(info[0], FIELD_LENGTH, stdin);
            info[0][strlen(info[0]) - 1] = 0;

            printf("Enter the departure location:\n");
            fgets(info[1], FIELD_LENGTH, stdin);
            info[1][strlen(info[1]) - 1] = 0;

            printf("Enter the destination:\n");
            fgets(info[2], FIELD_LENGTH, stdin);
            info[2][strlen(info[2]) - 1] = 0;

            printf("Enter by which Airline(FK):\n");
            fgets(info[3], FIELD_LENGTH, stdin);
            info[3][strlen(info[3]) - 1] = 0;

            insert_s(info);
        }
        else if (choice == 5) { //Edit-m
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            printf("Enter new country:\n");
            fgets(info[0], FIELD_LENGTH, stdin);
            info[0][strlen(info[0]) - 1] = 0;

            edit_m(key, info);
        }
        else if (choice == 6) { //Edit-s
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            printf("Enter new departure location:\n");
            fgets(info[0], FIELD_LENGTH, stdin);
            info[0][strlen(info[0]) - 1] = 0;

            printf("Enter new destination:\n");
            fgets(info[1], FIELD_LENGTH, stdin);
            info[1][strlen(info[1]) - 1] = 0;

            edit_s(key, info);
        }
        else if (choice == 7) { //Delete-m
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            //printf("Key = %s (hash = %d)\n", key, hash(key));
            //long int pos = search_pos_tind(&airlines, hash("AirLine0000"));
            //printf("pos = %d, hash = %d\n", pos, hash("AirLine0000"));

            delete_m(key);
        }
        else if (choice == 8) { //Delete-s
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            delete_s(key);
        }
        else if (choice == 9) {
            print_db();
        }
        else if (choice == 10) {
            printf("Total records: %d\n", airlines.count + flights.count);
        }
        else if (choice == 11) {
            printf("Main records: %d\n", airlines.count);
        }
        else if (choice == 12) {
            printf("Subrecords: %d\n", flights.count);
        }
        else if (choice == 13) {
            printf("Enter the key:\n");
            fgets(key, FIELD_LENGTH, stdin);
            key[strlen(key) - 1] = 0;

            printf("Subrecords for this main record: %d\n", count_s_for_m(key));
        }

        endl();
    }
    


    for(int i=0;i<4;++i)
        free(info[i]);
    free(key);
}

void fill_in(const long int n, const long int m) {
    char* inf[4];
    for (int i = 0; i < 4; ++i) {
        inf[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
    }

    char** names=(char**)malloc(sizeof(char*)*(m+n));
    for (int i = 0; i < n; ++i) {
        names[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
        sprintf_s(names[i], FIELD_LENGTH, "Airline%04d" ,i);
    }

    for (int i = n; i < n+m; ++i) {
        names[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
        sprintf_s(names[i], FIELD_LENGTH, "Flight%04d", i-10);
    }

    for (int i = 0; i < n; ++i) {
        strcpy(inf[0], names[i]);
        strcpy(inf[1], "Country");

        insert_m(inf);
        
    }
    

    srand((unsigned)time(NULL));

    for (int i = n; i < n+m; ++i) {
        strcpy(inf[0], names[i]);
        for (int j = 1; j < 3; ++j)
            strcpy(inf[j], "City");
                
        int k = (int)(((double)rand() / RAND_MAX) * (n-1));
        strcpy(inf[3], names[k]);  

        insert_s(inf);
    }

    for (int i = 0; i < 4; ++i)
        free(inf[i]);
    for (int i = 0; i < n+m; ++i)
        free(names[i]);
    free(names);
}

int main() {
    open_db();

    //fill_in(20, 40);
    //print_db();
    
    interact();
    
    close_db();

    system("pause");
    return 0;
}