#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

/*
   Print int a: printf("%d\n",a); printf("%i\n", a);

   char file_name[256]="rwtest.tmp";
   i_wfile=open(file_name,O_CREAT | O_RDWR );
   close(i_wfile)==-1 -> Error

   chsize(i_wfile,l_test_file_size)==-1) -> Errors with changing size

   write(i_wfile,pc_iobuf,l_buf_size);

   lseek(i_wfile,0,SEEK_SET)
*/

//TODO: init db, open db, close db, correct names, add const


//const int MULTIPLIER = 10;
const int MANAGER_SIZE = 8; // 2 long ints
const long int RECORDS_PER_UNIT = 50;
const unsigned long int Q = 114750541; //hash constant
const unsigned long int H = 4751951; //hash constant
const int FIELD_LENGTH = 20;

void endl() { printf("\n"); }
typedef struct  {
    char present; // 1 - present, 0 - deleted
    char name[20];
    char country[20];
    long int indSub;
} Airline;
void airline_print(Airline* a) {
    printf("%d %s\n", a->present, a->name);
    printf("\t%s\n", a->country);
    printf("%d\n", a->indSub);
}

typedef struct {
    char present; // 1 - present, 0 - deleted
    char code[20];
    char to[20];
    char from[20];
    long int ind_main;
    long int next;
} Flight;
void flight_print(Flight* f) {
    printf("%d %s\n", f->present, f->code);
    printf("\t%s\n", f->from);
    printf("\t%s\n", f->to);
    printf("%d\n", f->ind_main);
    printf("%d\n", f->next);
}

const long int MAX_DELETION = 5;


char im[256] = "index_main.db";
char is[256] = "index_sub.db";
char tm[256] = "table_main.db";
char ts[256] = "table_sub.db";
char mm[256] = "manager_main.db";
char ms[256] = "manager_sub.db";

long int hash(char* str) {
    if (str == "")
        return -1;
    
    unsigned long int hash = 0;
    for (int i = 0; (i < FIELD_LENGTH-1) && str[i]; ++i)
        hash = (H * hash + str[i]) % Q;
    return hash % Q;
}

void rand_str(char* w) {
    char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    srand((unsigned)time(NULL));
    for (int i = 0; i < FIELD_LENGTH - 1; ++i) {
        int k = (int)((sizeof(alphanum) - 2)*((double)rand() / RAND_MAX));
        w[i] = alphanum[k];
    }
}

/*
 * @note: For different types of files (e.g. table and index) unit size varies. Pass 0 to bytes_per_unit to create manager file
 */
int open_file(char* file_name, const long int bytes_per_unit) {
    int file_id = open(file_name, O_RDWR);
    if (file_id == -1) {
        file_id = open(file_name, O_CREAT | O_RDWR);
        close(file_id);        
        file_id = open(file_name, O_RDWR);
        
        if (bytes_per_unit != 0)
            chsize(file_id, RECORDS_PER_UNIT*bytes_per_unit);
        
        else {
            long int ma = 1;
            chsize(file_id, (long int) MANAGER_SIZE*ma);
            
            //fill with starter parameters
            long int p = -1;
            if (lseek(file_id, 0, SEEK_SET) < 0){          
                printf("lseek error\n");
            }
            write(file_id, &p, sizeof(long int));
            p = 2;
            lseek(file_id, 8, SEEK_SET);
            write(file_id, &p, sizeof(long int));
        }
    }
    return file_id;
}


typedef struct {
    char present; // 1 - present, 0 - deleted
    unsigned long int PK;
    long int ind;
} IndexCell;

typedef struct {
    int fid; //file for indexes
    int ftbl; //table
    int fmng; //file for resource manager
    IndexCell* ind;
    long int garbage[5];
    long int deletion_manager;
    long int next_line_pos;
    long int total;
    long int prs_count;
} Table;
Table airlines, flights;



void print_indt(Table* t) {
    printf("Present - PK - index\n");
    for (int i = 0; i < t->total; ++i)
        printf("%d - %d - %d\n", t->ind[i].present, t->ind[i].PK, t->ind[i].ind);
    endl();
}

void print_data_m() {   
    printf("AIRLINES: DATA: present, name, country, start of sublist"); endl();
    for (int i = 0; i < airlines.total; ++i) {
        Airline a;
        lseek(airlines.ftbl, sizeof(Airline)*i, SEEK_SET);
        read(airlines.ftbl, &a, sizeof(Airline));

        printf("-------------------------"); endl();
        airline_print(&a);
        printf("-------------------------"); endl();
    }
}

void print_data_s() {
    printf("FLIGHTS: DATA: present, code, from, to, main item, next in sublist"); endl();
    for (int i = 0; i < flights.total; ++i) {
        Flight f;
        lseek(flights.ftbl, sizeof(Flight)*i, SEEK_SET);
        read(flights.ftbl, &f, sizeof(Flight));

        printf("-------------------------"); endl();
        flight_print(&f);
        printf("-------------------------"); endl();
    }
}

long int add_to_sublist(long int index_main, long int index_sub) {
    Airline a;
    lseek(airlines.ftbl, index_main * sizeof(Airline), SEEK_SET);
    read(airlines.ftbl, &a, sizeof(Airline));

    long int next_sub = a.indSub;
    a.indSub = index_sub;
    
    lseek(airlines.ftbl, index_main * sizeof(Airline), SEEK_SET);
    write(airlines.ftbl, &a, sizeof(Airline));

    return next_sub;
}

void adjust_mng_info_after_insert(Table* t, long int ind) {
    if (ind != t->next_line_pos) {
        t->garbage[t->deletion_manager - 1] = -1;
        --t->deletion_manager;
    }
    else
        ++t->next_line_pos;

    ++t->total;
    ++t->prs_count;
}

//TODO: check if all memory is used
long int get_insert_pos(Table* t) {
    long int pos;   
    if (t->deletion_manager > 0) {
        pos = t->garbage[t->deletion_manager - 1];     
        return pos;
    } 
    else {     
        pos = t->next_line_pos;       
        return pos;
    }
}

//returns first non-lesser
long int binary_search(IndexCell* itbl, unsigned long int key, long int left, long int right) {
    if (left >= right) {        
        if(itbl[left].PK<key)
            return left+1;
        
        return left;
    }
    
    long int m = (long int)((left + right) / 2);  
    if (itbl[m].PK == key)
        return m;
    
    if (itbl[m].PK < key)
        binary_search(itbl, key, m+1, right);
    else
        binary_search(itbl, key, left, m-1);
}

//during insertion checks for uniqueness
int insert_to_indtbl(Table* t, IndexCell* cell) {
    long int pos = 0;
    if (t->total) //if there're another items we should search for appropriate position
        pos = binary_search(t->ind, cell->PK, 0, t->total - 1);
    else { //it's the first item
        t->ind[0] = *cell;
        adjust_mng_info_after_insert(t, cell->ind);

        return 0;
    }

    //check for uniqueness
    if ((pos < t->total)
        && (t->ind[pos].PK == cell->PK))
    {
        return -1;
    }

    if (t->total == 10)
        endl();

    //if item's PK is in the middle, we should shift greater PKs
    if (pos < t->total) {
        for (int i = t->total; i > pos; --i)
            t->ind[i] = t->ind[i - 1];
    }
    t->ind[pos] = *cell; 
    adjust_mng_info_after_insert(t, cell->ind);

    return 0;
}

long int search_pos_indt(Table* t, char* key) {
    unsigned long int h = hash(key);
    long int ind = binary_search(t->ind, h, 0, t->total - 1);

    if ((ind >= t->total)
        || ((ind < t->total) && (t->ind[ind].PK != h)))
    {
        printf("Error: No such item in database\n");
    }
    return ind;
}

long int search(Table* t, char* key) {
    long int ind = search_pos_indt(t, key);
    return ind != -1? t->ind[ind].ind : -1;
}

void remove_from_sublist(long int index) {
    Flight f;
    lseek(flights.ftbl, sizeof(Flight)*index, SEEK_SET);
    read(flights.ftbl, &f, sizeof(Flight));

    long int next_ind = f.next;

    Airline a;
    lseek(airlines.ftbl, sizeof(Airline)*f.ind_main, SEEK_SET);
    read(airlines.ftbl, &a, sizeof(Airline));

    if (a.indSub == index) {
        a.indSub = next_ind;
        lseek(airlines.ftbl, sizeof(Airline)*f.ind_main, SEEK_SET);
        read(airlines.ftbl, &a, sizeof(Airline));
        return;
    }

    long int it = a.indSub;
    while (f.next != index) {
        lseek(flights.ftbl, sizeof(Flight)*it, SEEK_SET);
        read(flights.ftbl, &f, sizeof(Flight));
        it = f.next;
    }

    f.next = next_ind;
    write(flights.ftbl, &f, sizeof(Flight));
}

void remove_from_indtbl(Table* t, long int pos) {
    t->ind[pos].present = 0;
    t->prs_count--;
    t->deletion_manager++;
    t->garbage[t->deletion_manager - 1] = t->ind[pos].ind;
}





int insert_m(char* info[]) {   
    long int pos = get_insert_pos(&airlines);
    
    Airline a;
    a.present = 1;
    a.indSub = -1;
    strcpy(a.name, info[0]);
    strcpy(a.country, info[1]);
    
    unsigned long int h = hash(a.name);

    IndexCell cell;
    cell.present = 1;
    cell.PK = h;
    cell.ind = pos;
    int success = insert_to_indtbl(&airlines, &cell);
    if (success == -1) {
        printf("Error: There's an item with same PK\n");
        return -1;
    }

    lseek(airlines.ftbl, sizeof(Airline) * pos, SEEK_SET);
    write(airlines.ftbl, &a, sizeof(Airline));

   // printf("Main item inserted %s\n", a.name);

    return 0;
}

int insert_s(char* info[]) {
    long int pos = get_insert_pos(&flights);

    Flight f;
    f.present = 1;
    strcpy(f.code, info[0]);
    strcpy(f.from, info[1]);
    strcpy(f.to, info[2]);

    unsigned long int h = hash(f.code);
    //printf("Hash of potential subitem %d\n", h);
    unsigned long int main_h = hash(info[3]);
    
    //printf("While inserting subitem, searching for %s\n", info[3]);
    long int ind_main = search(&airlines, info[3]);
    if (ind_main == -1)
        return -1;
    f.ind_main = ind_main;
   // printf("Found main item in %d\n",ind_main);

    IndexCell cell;
    cell.present = 1;
    cell.PK = h;
    cell.ind = pos;
    int success = insert_to_indtbl(&flights, &cell);
    //printf("Insertion success: %d\n", success);
    if (success == -1) {
        printf("Error: There's an item with same PK or something else\n");
        return -1;
    }
    f.next = add_to_sublist(ind_main, pos);

    lseek(flights.ftbl, sizeof(Flight) * pos, SEEK_SET);
    write(flights.ftbl, &f, sizeof(Flight));

    //printf("Sub item inserted with PK %d\n", h); endl();

    return 0;
}

Airline* get_mitem(char* key) {
    long int ind = search(&airlines, key);
    if (ind == -1)
        return NULL;

    Airline* a = (Airline*)malloc(sizeof(Airline));
    lseek(airlines.ftbl, sizeof(Airline)*ind, SEEK_SET);
    read(airlines.ftbl, a, sizeof(Airline));

    return a;
}

Flight* get_sitem(char* key) {
    long int ind = search(&flights, key);
    if (ind == -1)
        return NULL;

    Flight* f = (Flight*)malloc(sizeof(Flight));
    lseek(flights.ftbl, sizeof(Flight)*ind, SEEK_SET);
    read(flights.ftbl, f, sizeof(Flight));

    return f;
}

long int count_m() {
    return airlines.prs_count;
}

long int count_s() {
    return flights.prs_count;
}

long int count_s_for_m(char* key) {
    long int ind = search(&airlines, key);
    if (ind == -1)
        return -1;
    
    Airline a;
    lseek(airlines.ftbl, sizeof(Airline)*ind, SEEK_SET);
    read(airlines.ftbl, &a, sizeof(Airline));

    long int total = 0;
    long int sub_ind = a.indSub;

    Flight f;
    while (sub_ind != -1) {
        ++total;
        lseek(flights.ftbl, sizeof(Airline)*sub_ind, SEEK_SET);
        read(flights.ftbl, &f, sizeof(Airline));
        sub_ind = f.next;
    }

    return total;
}

int delete_s(char* key) {
    long int pos = search_pos_indt(&flights, key);
    if (pos == -1)
        return -1;

    long int ind = flights.ind[pos].ind;

    remove_from_sublist(ind);
    remove_from_indtbl(&flights, pos);

    Flight f;
    lseek(flights.ftbl, sizeof(Flight)*ind, SEEK_SET);
    read(flights.ftbl, &f, sizeof(Flight));
    f.present = 0;
    write(flights.ftbl, &f, sizeof(Flight));
    
    return 0;
}

int delete_m(char* key) {
    long int pos = search_pos_indt(&airlines, key);
    if (pos == -1)
        return -1;
    
    remove_from_indtbl(&airlines, pos);
    
    long int ind = airlines.ind[pos].ind;
    Airline a;
    lseek(airlines.ftbl, sizeof(Airline)*ind, SEEK_SET);
    read(airlines.ftbl, &a, sizeof(Airline));
    a.present = 0;
    write(airlines.ftbl, &a, sizeof(Airline));

    Flight f;
    long int it = a.indSub;
    long int sub_pos;
    while (it != -1) {
        lseek(flights.ftbl, sizeof(Flight)*it, SEEK_SET);
        read(flights.ftbl, &f, sizeof(Flight));
        f.present = 0;
        write(flights.ftbl, &f, sizeof(Flight));

        sub_pos = binary_search(flights.ind, hash(f.code), 0, flights.total - 1);
        remove_from_indtbl(&flights, sub_pos);

        it = f.next;
    }
    
}




void print_db() {
    print_indt(&airlines);
    print_data_m();
    endl(); endl();
    print_indt(&flights);
    print_data_s();
}

int main() {
    flights.ftbl = open(ts, O_CREAT | O_RDWR);
    chsize(flights.ftbl, 500);
    //close(flights.ftbl);
    //flights.ftbl= open(ts, O_RDWR);   
    flights.total = 0;
    flights.prs_count = 0;
    flights.deletion_manager = 0;
    flights.next_line_pos = 0;
    flights.ind = (IndexCell*)malloc(RECORDS_PER_UNIT * sizeof(IndexCell));
    
    airlines.ftbl = open(tm, O_CREAT | O_RDWR);
    chsize(airlines.ftbl, 500);
    //close(airlines.ftbl);
    //airlines.ftbl = open(tm, O_RDWR);
    airlines.total = 0;
    airlines.prs_count = 0;
    airlines.deletion_manager = 0;
    airlines.next_line_pos = 0;
    airlines.ind = (IndexCell*)malloc(RECORDS_PER_UNIT * sizeof(IndexCell));

    char* inf[4];
    for (int i = 0; i < 4; ++i) {
        inf[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
    }

    char* random_words[30];
    for (int i = 0; i < 30; ++i) {
        random_words[i] = (char*)malloc(FIELD_LENGTH * sizeof(char));
        sprintf_s(random_words[i],20, "item%04d" ,i);
    }

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 4; ++j)
            strcpy(inf[j], random_words[i]);

        insert_m(inf);
    }

    srand((unsigned)time(NULL));

    for (int i = 10; i < 30; ++i) {
        for (int j = 0; j < 3; ++j)
            strcpy(inf[j], random_words[i]);
                
        int k = (int)(((double)rand() / RAND_MAX) * 9);
        //printf("random number %d\n", k);
        strcpy(inf[3], random_words[k]);  

        insert_s(inf);
    }

    print_db();



    close(flights.ftbl);
    close(airlines.ftbl);

    system("pause");
    return 0;
}