#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define NUM_MAX_PROCESSES 3
#define NUM_MAX_PAGES 10
#define NUM_MAX_FRAMES 64
#define NUM_MAX_REQUESTS 7

#define WORKING_SET_LIMIT 4

//CORES
#define RED "\033[1;31m"
#define GREEN "\033[0;32m"
#define BLUE "\033[0;34m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[0;36m"
#define PURPLE "\033[0;35m"
#define RESET "\033[0m"

/*

TODO:
- como a pagetable pode ser implementada?
- tirar um processo inteiro da memoria principal, ou somente uma pagina quando uma pagina nova deslocar uma pagina da memoria principal?
- quando imprimir o trace?
- como implementar a memoria swap?

PREMISSAS:
Um processo termina apos realizar 20 requisicoes.
A memoria eh representada pela lista de frames.

*/

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
	process_control_block pcb;
	int num_total_requests, num_requests_done;
	int t_added_to_memory;
	int working_set_size;
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

void l_shift(page *working_set, int id);
void LRU(process* p, int id);

/**************************** CREATION FUNCS *******************************/

process* new_process(int id) {
	printf(BLUE "---new_process---\n" RESET);

	process* p = (process *) malloc(sizeof(process));
	
	p->pcb.process_id = id;
	p->pcb.t_created = instante_tempo_global;
	p->num_total_requests = NUM_MAX_REQUESTS;
	p->num_requests_done = 0;
	p->t_added_to_memory = instante_tempo_global;
	
	
	p->working_set_size = 0;
	p->working_set = (page *) calloc(sizeof(page), WORKING_SET_LIMIT);
	
	p->page_list.size = 0;
	p->page_list.first = NULL;
	p->page_list.last = NULL;
	
	p->previous = NULL;
	p->next = NULL;
	
	initiate_process_pages(p);
	
	printf(BLUE"---ENDnew_process---\n"RESET);
	return p;
}

page* new_page(int parent_id, int page_id) {
	page* p = (page *) malloc(sizeof(page));
	
	p->t_last_access = -1;
	p->page_id = page_id;
	p->parent_process_id = parent_id;
	
	p->previous = NULL;
	p->next = NULL;
	
	return p;
}

frame* new_frame(int frame_id) {
	frame* f = (frame *) malloc(sizeof(frame));
	
	f->frame_id = frame_id;
	f->page = NULL;
	f->previous = NULL;
	f->next = NULL;
	
	return f;
}

/**************************** INITIALIZATION FUNCS *******************************/

void create_frames() {
	
	frame* f = new_frame(new_frame_ID);
	new_frame_ID++;
	
	if(MEMORY.size != 0) {
		f->previous = MEMORY.last;
		MEMORY.last->next = f;
		MEMORY.last = f;
	}
	else {
		MEMORY.first = f;
		MEMORY.last = f;
	}
	
	MEMORY.size++;
	
}

void create_process() {
	printf(BLUE "---create_process---\n" RESET);
	
	process* p = new_process(new_process_ID);
	new_process_ID++;
	
	if(PROCESS_LIST.size != 0) {
		p->previous = PROCESS_LIST.last;
		PROCESS_LIST.last->next = p;
		PROCESS_LIST.last = p;
	}
	else {
		PROCESS_LIST.first = p;
		PROCESS_LIST.last = p;
	}
	
	PROCESS_LIST.size++;
	
	printf(BLUE "---ENDcreate_process---\n" RESET);
}

void initiate_process_pages(process* p) {
	for(int i = 0; i < NUM_MAX_PAGES; i++)
		insert_page(&p->page_list, i+1, p->pcb.process_id);
}

/**************************** FRAME FUNCS *******************************/

//pega a primeira frame disponivel
frame* get_available_frame(frame_list* fl) {
	frame* f = fl->first;
	if(f != NULL) {
		while(f->next != NULL) {
			f = f->next;
		}
	}

	return f;
}

void insert_page_into_frame(page* p, frame* f) {
	f->page = p;
}

/**************************** PAGE & PAGELIST FUNCS *******************************/

void insert_page(page_list* pl, int page_id, int process_id) {
	page* p = pl->last;
	page* new = new_page(process_id, page_id);
	
	if(pl->size == 0) {
		pl->first = new;
	}
	else {
		p->next = new;
		new->previous = p;
	}
	
	pl->last = new;
	pl->size++;
}

//nao testada
void remove_page(page_list* pl, int page_id) {
	page* p = pl->first;
	
	while(p != NULL && p->page_id != page_id) {
		p = p->next;
	}
	
	if(p == NULL) {
		puts(RED "pagina nao esta na lista, nao pode ser removida" RESET);
	}
	else {
		p->previous->next = p->next;
		p->next->previous = p->previous;
		free(p);
		pl->size--;
	}
}

void replace_page(page* working_set, page p) {
	working_set[0] = p;
}

page* get_page(page* working_set, int page_id) {
	page* p = NULL;
	
	for(int i = 0; i < WORKING_SET_LIMIT; i++) {
		if(page_id == working_set[i].page_id)
			p = &working_set[i];
	}
	
	return p;
}

page* get_page_from_page_list(page_list* pl, int page_id) {
	page* p = pl->first;
	
	while(p != NULL && p->page_id != page_id) {
		p = p->next;
	}
	
	if(p == NULL)
		return NULL;
	else
		return p;
}

/**************************** PRINT FUNCS *******************************/

void print_infos() {
	process* p = PROCESS_LIST.first;
	//frame* f = MEMORY.first;
	
	puts(GREEN "Process Info:" RESET);
	while(p != NULL) {
		print_process(p);
		p = p->next;
	}
}

void print_page(page p) {
	printf(BLUE "---print_page---\n" RESET);
	
	printf("p.t_last_access: %d ", p.t_last_access);
	printf("p.page_id: %d ", p.page_id);
	printf("p.parent_process_id: %d \n", p.parent_process_id);
	
	printf(BLUE "---ENDprint_page---\n" RESET);
}

void print_page_list(page_list pl) {
	printf(BLUE "---print_page_list---" RESET);
	
	printf("pl.size: %d ", pl.size);
	if(pl.first != NULL)
		printf("pl.first->page_id: %d ", pl.first->page_id);
	if(pl.last != NULL)
		printf("pl.last->page_id: %d ", pl.last->page_id);
	
	printf(BLUE "---ENDprint_page_list---" RESET);
}

void print_process(process* p) {
	printf("Process ID: %d\n", p->pcb.process_id);
	printf("Requests Done: %d\n", p->num_requests_done);
    printf("Working Set: ");
	for(int i = 0; i < WORKING_SET_LIMIT; i++)
		printf("%d | ", p->working_set[i].page_id);
	puts("\n-----------------------------------------");
}

/**************************** PROCESS FUNCS *******************************/

void request_page(process* p) {	
	int random_page_id = random()%NUM_MAX_PAGES + 1;
	page* found_page = get_page(p->working_set, random_page_id);

	printf(BLUE "Process #%d makes Page #%d request\n" RESET, p->pcb.process_id ,random_page_id);

   	if(found_page != NULL) {
     	l_shift(p->working_set, found_page->page_id);
   	}
   	else {
     	page* pg = get_page_from_page_list(&p->page_list, random_page_id);
     	
     	if(p->working_set_size == WORKING_SET_LIMIT) {
        	p->working_set[0] = *pg;
     	}
     	else {
        	LRU(p, random_page_id);
        	
        	if(available_frames > 0) {
          		frame* f = get_available_frame(&MEMORY);
          		if(pg != NULL) {
          			f->page = pg;
          		}
        	} else {
                process* p = PROCESS_LIST.first;
                frame* fr = MEMORY.first;
                
                while(fr != NULL) {
                    if(fr->page->parent_process_id == p->pcb.process_id)
                        free(fr->page);
                    fr = fr->next;
                }
                PROCESS_LIST.first->next->previous = NULL;
                PROCESS_LIST.first = PROCESS_LIST.first->next;
        }
     }
   }
   p->num_requests_done++;
}

/***************************** UTILITY FUNCTIONS ********************************/

void update_flags() {
	
	if(instante_tempo_global % 3 == 0) {
		printf(GREEN "New Requests were made\n" RESET);
		time_to_create_process = 1;
		time_to_request_page = 1;
		//printf("time_to_create_process e time_to_request_page: %d %d \n", time_to_create_process, time_to_request_page);
	}
	else {
		time_to_create_process = 0;
		time_to_request_page = 0;
	}

}

/***************************** LRU FUNCTIONS ********************************/

void l_shift(page *working_set, int id) {

    int index = 3;
    for(int i = 3; i >= 0; i--)             //buscar indice da ultima posicao ocupada (evitar os 0s)
        if(working_set[i].page_id == 0)
            index = i-1;

    for(int i = 0; i < 4; i++) {

        if(working_set[i].page_id == id) {

            int temp = working_set[i].page_id;
            for(int j = i; j < index; j++)
                working_set[j] = working_set[j+1];

            working_set[index].page_id = temp;

        }

    }

}

/*
 * recebe o numero da pagina e o working set do processo/thread
 * se houver espaço, adiciona a pagina ao fim do working set
 * caso contrario, remove a pagina utilizada a mais tempo (least recently used)
 */
void LRU(process* p, int id) {
    page* pg = get_page_from_page_list(&p->page_list, id);

    for(int i = 0; i < 4; i++) {
        if(p->working_set[i].page_id == pg->page_id) {                 //pagina ja esta presente no working set
            l_shift(p->working_set, pg->page_id);                      //atualiza working set
            return;
        } else if(p->working_set[i].page_id == 0) {                    //nova pagina && existe frame vazio no working set
            p->working_set[i].page_id = pg->page_id;                   //primeiro frame vazio recebe a pagina
            return;
        }     
    }
    /* pagina nao esta presente e nao ha espaco vazio no working set: */
    p->working_set[0].page_id = pg->page_id;
    l_shift(p->working_set, pg->page_id);
}

/* PSEUDOCODIGO / FUNCIONAMENTO / IDEIAS */

//INSERTION INTO FRAME
//se existe alguma frame disponivel
//	inserir_pag()
//else
//	remover o processo mais antigo da memoria
//	inserir pagina do processo
//
//CODE:
//if frames_available > 0:
//	pass








//PROCESSO DE REMOCAO DO PROCESSO MAIS ANTIGO:
//	mais antigo = INT_MAX
//	frame mais antigo = -1
//	frame f = MEMORY.first
//	while(f != NULL)
//		process_id = f->page->parent_process_id
//		p = get_process(process_id)
//		if p->t_added_to_memory < mais antigo
//			mais antigo = p->t_added_to_memory
//			frame mais antigo = f
//		f = f->next
//	free_frame(frame mais antigo)



//COMO FUNCIONA O PROCESSO ESTAR OU NAO NA MEMORIA VIRTUAL?
//	uma frame eh ocupada por uma pagina que contem o id do processo pai



//COMO IMPLEMENTAR A PAGETABLE?
//um array de duas dimensoes pagetable[50][2], onde o primeiro campo indica se a pag esta na memoria e a segunda indica o numero da frame (se pag esta na memoria)



//COMO IMPLEMENTAR UMA LRU QUEUE PARA PROCESSOS?
//a fila de processos comeca vazia
//o primeiro processo eh inserido na primeira posicao
//	o primeiro processo faz uma requisicao de pagina
//o segundo processo eh inserido na segunda posicao
//	o primeiro processo faz uma requisicao de pag
//	o segundo proc faz uma req de pag
//
//neste ponto, os tempos de ultimo acesso de qualquer pag dos dois procs sao iguais (nao importa qual seria retirado)
//NAO SEI SE UTILIZAR O TEMPO DE ULTIMA REQUISICAO SERIA UMA BOA IDEIA, PQ TODOS OS PROCESSOS FAZEM REQUISICOES A CADA 3 SEGS
//O QUE UTILIZAR?

/***************************** MAIN ********************************/

int main(int argc, char* argv[]) {
	
	srand(time(NULL));
	
	process* p;
	
	while(MEMORY.size < NUM_MAX_FRAMES)
		create_frames();
	
	while(1) {
		printf("\n\n================= CURRENT TIME UNIT: %d ============== \n", instante_tempo_global);
		update_flags();
		
		if(time_to_create_process && PROCESS_LIST.size < NUM_MAX_PROCESSES) {
			printf("\n\n ITG: %d\n\n", instante_tempo_global);
			create_process();
		}
		
		if(time_to_request_page) {
			p = PROCESS_LIST.first;
			while(p != NULL) {
				request_page(p);
				p = p->next;
			}
		}
		
		print_infos();
		
		instante_tempo_global++;
		
		process* check;
		check = PROCESS_LIST.first;
		while(check != NULL) {
			if(check->num_requests_done == NUM_MAX_REQUESTS) {
				processes_done_counter++;
				PROCESS_LIST.first = PROCESS_LIST.first->next;
			}
			check = check->next;
		}
		if(processes_done_counter == NUM_MAX_PROCESSES)
			break;
	}

    return 0;
}

/*
TODO:
	-swap
	-trace
	-tabela de paginas
	-exemplo simples do programa
	-relatorio
	-buscar referencias
*/