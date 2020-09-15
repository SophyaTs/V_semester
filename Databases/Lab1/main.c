#include <stdio.h>
#include <fcntl.h>

/*
   Print int a: printf("%d\n",a); printf("%i\n", a);

   char file_name[256]="rwtest.tmp";
   i_wfile=open(file_name,O_CREAT | O_RDWR );
   close(i_wfile)==-1 -> Error

   chsize(i_wfile,l_test_file_size)==-1) -> Errors with changing size

   write(i_wfile,pc_iobuf,l_buf_size);

   lseek(i_wfile,0,SEEK_SET)
*/

//struct Airline {
//    char name[20];
//    char country[20];
//};
//
//struct Flight {
//    char code[10];
//    char to[20];
//    char from[20]
//};

const long int MIN_RECORDS = 10;
const int MULTIPLIER = 10;
const int INDEX_SIZE = 13; //1 - present, 4 - hash, 8 - relation
const int MAIN_OBJ_SIZE = 49; // 1 - present, 20+20 data, 8 relation
const int SUBOBJ_SIZE = 79; // 1 - present, 20+20+20 data, 8 relation, 8 next
const int MANAGER_SIZE = 16; // 2 long ints
const int Q = 12239; //hash constant
const int FIELD_LENGTH = 20;

const int MAX_DELETION = 5;
int deletion_manager = 0;

char im[256] = "1_index_main.db";
char is[256] = "4_index_sub.db";
char tm[256] = "2_table_main.db";
char ts[256] = "5_table_sub.db";
char mm[256] = "3_manager_main.db";
char ms[256] = "6_manager_sub.db";

int hash(char* str) {
    if (str == "")
        return -1;
    
    int hash = 0;
    for (int i = 0; (i < FIELD_LENGTH-1) && str[i]; ++i)
        hash = (256 * hash + str[i]) % Q;
    return hash % Q;
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
            chsize(file_id, MIN_RECORDS*bytes_per_unit);
        else {
            long int ma = 1;
            chsize(file_id, (long int) MANAGER_SIZE*ma);
            
            //fill with starter parameters
            long int p = -1;
            if (lseek(file_id, 0, SEEK_SET) < 0)
            {
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


struct Table {
    int ind; //indexes
    int tbl; //table
    int mng; //janitor
} airlines, flights;


long int search_by_PK(struct Table* t, char* key) {
    int h = hash(key);
    if (h == -1)
        return -1;

    int end = 0;
    lseek(t->mng, 3, SEEK_SET);
    read(t->mng, &end, 4);

    int hi = 0;
    for (long int i = 0; INDEX_SIZE * i < end*INDEX_SIZE - 1; ++i) {
        lseek(t->tbl, 1 + INDEX_SIZE*i, SEEK_SET);
        read(t->tbl, &hi, 4);

        if (hi == h)
            return i;
    }

    return -1;
}

long int get_insrt_pos(struct Table* t) {
    long int pos;
    
    lseek(t->mng, 0, SEEK_SET);
    read(t->mng, &pos, 8);

    if (pos != -1) {
        write(t->mng, (long int) -1, 8);
        --deletion_manager;
    }
    else {
        lseek(t->mng, 3, SEEK_SET);
        read(t->mng, &pos, 8);
        write(t->mng, pos+1, 8);
    }

    return pos;
}

void add_to_sublist(long int index_main, long int key) {
    lseek(airlines.tbl, index_main*MAIN_OBJ_SIZE+2*FIELD_LENGTH, SEEK_SET);
    long int ptr = 0;
    ptr = read(airlines.tbl, ptr, 8);
    write(airlines.tbl, key, 8);

    lseek(flights.tbl, key*SUBOBJ_SIZE + 3 * FIELD_LENGTH + 8, SEEK_SET);
    write(flights.tbl, ptr, 8);
}

/*
 * @note: to not create a relation pass ""
 */
//TODO: check for uniqueness
int add(struct Table* t, int varc, char* info[], char* relation) {
    int rel_ind = -1;
    
    //if trying to add subitem, check is there is main item
    if (t == &flights) {
        rel_ind = search_by_PK(t == &airlines ? &flights : &airlines, relation);
        if (rel_ind == -1)
            return -1;
    }

    int unit_size = t == &airlines ? MAIN_OBJ_SIZE : SUBOBJ_SIZE;
    int pos = get_insrt_pos(t);

    //adding new item to index list
    int seek_pos = INDEX_SIZE * pos - 1;
    lseek(t->ind, seek_pos, SEEK_SET);
    write(t->ind, (char)1, 1);
    lseek(t->ind, seek_pos + 1, SEEK_SET);
    write(t->ind, hash(info[0]), 4);
    lseek(t->ind, seek_pos + 5, SEEK_SET);
    write(t->ind, rel_ind, 4);

    //filling the data in the table
    seek_pos = unit_size * pos - 1;
    lseek(t->tbl, seek_pos, SEEK_SET);
    write(t->tbl, (char)1, 1);
    for (int i = 0; i < varc; ++i) {
        lseek(t->tbl, seek_pos + 1 + FIELD_LENGTH*i, SEEK_SET);
        write(t->tbl, info[i], FIELD_LENGTH);
    }

    //specify relation
    seek_pos += FIELD_LENGTH * varc;
    lseek(t->tbl, seek_pos, SEEK_SET);
    write(t->tbl, rel_ind, 8);
    
    //if adding subitem, we extend the list of subitems 
    if (t == &flights)             
        add_to_sublist(rel_ind, pos);

    return 0;
}


int main() {
    airlines.ind = open_file(im, INDEX_SIZE);
    airlines.tbl = open_file(tm, MAIN_OBJ_SIZE);
    airlines.mng = open_file(mm, 0);

    flights.ind = open_file(is, INDEX_SIZE);
    flights.tbl = open_file(ts, MULTIPLIER* SUBOBJ_SIZE);
    flights.mng = open_file(ms, 0);
    
    close(airlines.ind);
    close(airlines.tbl);
    close(airlines.mng);
    close(flights.ind);
    close(flights.tbl);
    close(flights.mng);

    system("pause");
    return 0;
}