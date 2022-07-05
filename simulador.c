#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define NUM_MAX_PROCESSES 20
#define NUM_MAX_PAGES 50
#define NUM_MAX_FRAMES 64
#define NUM_MAX_REQUESTS 20

#define WORKING_SET_LIMIT 4

//CORES
#define RED "\033[1;31m"
#define GREEN "\033[0;32m"
#define BLUE "\033[0;34m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[0;36m"
#define PURPLE "\033[0;35m"
#define RESET "\033[0m"

/***************************** STRUCTURES ********************************/

typedef struct {
    int process_id, t_created;
} process_control_block;

typedef struct page {
	int t_last_access;
	int page_id;
	int parent_process_id;
	struct page* previous;
	struct page* next;
} page;

typedef struct {
	int size;
	page* first;
	page* last;
} page_list;

typedef struct process {
	int num_total_requests, num_requests_done;
	int t_added_to_memory;
	int working_set_size;
	process_control_block pcb;
    page* working_set;
    page_list page_list;
    struct process* previous;
    struct process* next;
} process;

typedef struct {
	int size;
	process* first;
	process* last;
} process_list;

typedef struct frame {
	int frame_id;
	page* page;
	struct frame* previous;
	struct frame* next;
} frame;

typedef struct {
	int size;
	frame* first;
	frame* last;
} frame_list;

/***************************** GLOBAL VARIABLES ********************************/

int instante_tempo_global = 0; // instante de tempo atual para todo o SO
int processes_done_counter = 0; 
int available_frames = 64;
int new_process_ID = 1;
int new_frame_ID = 1;
frame_list MEMORY;
//page_list SWAP_MEMORY;
process_list PROCESS_LIST;

//FLAGS
int time_to_create_process = 0;
int time_to_request_page = 0;

/***************************** FUNCTIONS CALLS ********************************/

process* new_process(int id);
page* new_page(int parent_id, int page_id);
frame* new_frame(int frame_id);
void create_frames();
void create_process();
int is_frame_available(frame* f);
void initiate_process_pages(process* p);
int page_index_in_array(page* array, int page_id);
void request_page(process* p);
void update_flags();
void print_infos();
void print_page(page p);
void print_process(process* p);

void insert_page(page_list* pl, int page_id, int process_id);
void remove_page(page_list* pl, int page_id);

void l_shift(int* working_set, int value);


/***************************** AUX FUNCTIONS ******************************/

void l_shift(int *working_set, int value) {

    int index = 3;
    for(int i = 3; i >= 0; i--)             //buscar indice da ultima posicao ocupada (evitar os 0s)
        if(working_set[i] == 0)
            index = i-1;

    for(int i = 0; i < 4; i++) {

        if(working_set[i] == value) {

            int temp = working_set[i];
            for(int j = i; j < index; j++)
                working_set[j] = working_set[j+1];

            working_set[index] = temp;

        }

    }

}

/*
void push(int *working_set, int value) {

    for(int i = 0; i < 4; i++)
        if(working_set[i] == 0)
            working_set[i] = value;

}
*/

/***************************** LRU ****************************************/

/*
 * recebe o numero da pagina e o working set do processo/thread
 * se houver espaço, adiciona a pagina ao fim do working set
 * caso contrario, remove a pagina utilizada a mais tempo (least recently used)
 */
void LRU(process* p) {

    int page_id = p->page_list.first->page_id;

    for(int i = 0; i < 4; i++) {

        if(p->working_set[i].page_id == page_id) {                 //pagina ja esta presente no working set

            l_shift(p->working_set, page_id);                      //atualiza working set
            return;

        } else if(p->working_set[i].page_id == 0) {                //nova pagina && existe frame vazio no working set

            p->working_set[i].page_id = page_id;                   //primeiro frame vazio recebe a pagina
            /* incrementar page fault */
            return;

        }                           

    }

    /* pagina nao esta presente e nao ha espaco vazio no working set: */
    p->working_set[0].page_id = page_id;
    l_shift(p->working_set, page_id);

    /* incrementar page fault */

}

int main() {

    process* p;

    LRU(p);

}