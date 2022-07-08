#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

/*

PREMISSAS:
Um processo termina apos realizar 20 requisicoes.
A memoria eh representada pela lista de frames.

As paginas sao armazenadas como doubly linked lists. Nos processos elas podem ser acessadas por meio de uma nova estrutura que possui 
um ponteiro para a primeira pagina, a ultima, e o numero de paginas naquela linked list.

Os frames e processos tambem sao armazenados como doubly linked lists. Eles devem ser acessados por variaveis globais, 
ISSO DEIXA PRO RELATORIO
*/

/***************************** STRUCTURES ********************************/

typedef struct {
    int process_id, t_created;
} process_control_block;

typedef struct page {
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
	int num_requests_done;
	int has_just_requested_page;
	int pagetable[NUM_MAX_PAGES][2];
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
int processes_done_counter = 0; // utilizado para saber quando terminar o programa
int created_processes = 0; // utilizado para saber se ainda faltam processos para criar
int new_process_ID = 1; // ID do proximo processo criado
int new_frame_ID = 1; // ID da proxima frame criada

frame_list MEMORY; // armazena todas as frames
process_list SWAP_MEMORY; // armazena todos os processo criados sem paginas carregadas na memoria
process_list PROCESS_LIST; // armazena todos os processo criados com pelo menos uma pagina carregada

//FLAGS
int time_to_create_process = 0;
int time_to_request_page = 0;
int any_process_done = 0;

FILE* trace; //arquivo de saida

/***************************** FUNCTION CALLS ********************************/

process* new_process(int id);
page* new_page(int parent_id, int page_id);
frame* new_frame(int frame_id);
void create_frames();
void create_process();
void create_page(process* p, int page_id);

frame* get_first_available_frame(frame_list* fl);
frame* get_frame_by_process_and_page(frame_list* fl, int page_id, int process_id);
void insert_frame(frame_list* fl, frame* new);
int is_any_frame_available();
void release_frame(frame* f);
void release_process_frames(process* p);

void insert_page(page_list* pl, page* new);
int is_page_in_working_set(page* working_set, int page_id);
page* get_page_from_page_list(page_list* pl, int page_id);

void request_page(process* p);
process* get_process(process_list* pl, int process_id);
void insert_process(process_list* pl, process* new);
process* remove_process(process_list* pl, int process_id);
void terminate_process(process* p, process_list* pl);
void terminate_done_processes();
process* get_different_process(process* p, process_list* pl);
void reset_working_set(process* p);
void reset_process_flags();
int is_working_set_full(process* p);

void update_flags();

void l_shift(page *working_set, int id);
void LRU(process* p, int id);

void swap_out(int process_id);
void swap_in(int process_id);

void print_infos();
void print_process(process* p);

/**************************** CREATION FUNCTIONS *******************************/

/*
 * Cria um novo processo. Seu working set eh inicializado vazio, todas as 
 * paginas do processo sao criadas nessa funcao e nao serao modificadas.
 * Os ponteiros 'previous' e 'next' somente sao inicializados quando o processo for inserido 
 * em alguma lista.
 */
process* new_process(int id) {
	process* p = (process *) malloc(sizeof(process));
	
	p->pcb.process_id = id;
	p->pcb.t_created = instante_tempo_global;
	p->num_requests_done = 0;
	p->has_just_requested_page = 0;
	for(int i = 0; i < NUM_MAX_PAGES; i++)
		for(int j = 0; j < 2; j++)
			p->pagetable[i][j] = 0;
	
	p->working_set = (page *) calloc(sizeof(page), WORKING_SET_LIMIT);
	
	p->page_list.size = 0;
	p->page_list.first = NULL;
	p->page_list.last = NULL;
	
	p->previous = NULL;
	p->next = NULL;

	for(int page_id = 1; page_id <= NUM_MAX_PAGES; page_id++)
		create_page(p, page_id);

	return p;
}

/*
 * Cria uma pagina. Ela deve ser criada para um processo especifico, com um ID unico dentro dele.
 * Os ponteiros 'previous' e 'next' somente sao inicializados quando o processo for inserido 
 * em alguma lista.
 */
page* new_page(int parent_id, int page_id) {
	page* p = (page *) malloc(sizeof(page));
	
	p->page_id = page_id;
	p->parent_process_id = parent_id;
	p->previous = NULL;
	p->next = NULL;
	
	return p;
}

/*
 * Cria uma frame.
 * Os ponteiros 'previous' e 'next' somente sao inicializados quando o processo for inserido 
 * em alguma lista.
 */
frame* new_frame(int frame_id) {
	frame* f = (frame *) malloc(sizeof(frame));
	
	f->frame_id = frame_id;
	f->page = NULL;
	f->previous = NULL;
	f->next = NULL;
	
	return f;
}

/*
 * Cria e insere uma frame na memoria. Utiliza uma variavel global 'new_frame_ID' para 
 * garantir um ID unico.
 */
void create_frames() {
	frame* f = new_frame(new_frame_ID);
	new_frame_ID++;
	insert_frame(&MEMORY, f);
}

/*
 * Cria e insere um processo na lista de processos. Utiliza uma variavel global 'new_process_ID' para 
 * garantir um ID unico.
 */
void create_process() {
	process* p = new_process(new_process_ID);
	new_process_ID++;
	insert_process(&PROCESS_LIST, p);
	created_processes++;
}

/*
 * Cria uma pagina e a insere na lista de paginas do processo passado como parametro.
 */
void create_page(process* p, int page_id) {
	page* pg = new_page(p->pcb.process_id, page_id);
	insert_page(&p->page_list, pg);
}

/**************************** FRAME FUNCTIONS *******************************/

/*
 * Retorna a primeira frame disponivel, ou NULL se todas estiverem ocupadas.
 */
frame* get_first_available_frame(frame_list* fl) {
	frame* f = fl->first;
	
	while(f != NULL && f->page != NULL)
		f = f->next;

	return f;
}

/*
 * Busca uma frame pela pagina que a ocupa (e processo dono da pagina).
 * Retorna NULL se a pagina nao estiver presente a frame_list.
 */
frame* get_frame_by_process_and_page(frame_list* fl, int page_id, int process_id) {
	frame *f = fl->first;

	while(f != NULL) {
		if(f->page != NULL && f->page->parent_process_id == process_id && f->page->page_id == page_id)
			break;
		f = f->next;
	}
	
	return f;
}

/*
 * Insere uma frame na frame list.
 */
void insert_frame(frame_list* fl, frame* new) {
	if(fl->size == 0) {
		fl->first = new;
	}
	else {
		fl->last->next = new;
		new->previous = fl->last;
	}
	
	fl->last = new;
	fl->size++;
}

/*
 * Retorna 1 se qualquer frame puder ser alocada a uma pagina. Retorna 0 se todas estao ocupadas.
 */
int is_any_frame_available() {
	frame* f = MEMORY.first;
	
	while(f != NULL) {
		if(f->page == NULL)
			return 1;
		f = f->next;
	}
	
	return 0;
}

/*
 * Desocupa a frame, permitindo que possa ser ocupada por outra pagina.
 */
void release_frame(frame* f) {
	if(f != NULL)
		f->page = NULL;
}

/*
 * Busca todas as frames que estiverem ocupadas por paginas no working set de um processo e as desocupam.
 */
void release_process_frames(process* p) {
	for(int i = 0; i < WORKING_SET_LIMIT; i++) {
		if(p->working_set[i].page_id != 0) {
			frame* f = get_frame_by_process_and_page(&MEMORY, p->working_set[i].page_id, p->pcb.process_id);
			if(f != NULL) {
				release_frame(f);
				fprintf(trace, "Frame #%d foi disponibilizada.\n", f->frame_id);
			}
		}
	}
}

/**************************** PAGE FUNCTIONS *******************************/

/*
 * Insere pagina em uma lista de paginas.
 */
void insert_page(page_list* pl, page* new) {
	if(pl->size == 0) {
		pl->first = new;
	}
	else {
		pl->last->next = new;
		new->previous = pl->last;
	}
	
	pl->last = new;
	pl->size++;
}

/*
 * Retorna 1 se uma pagina com id igual a 'page_id' esta no working set.
 * Retorna 0 se nao esta presente.
 */
int is_page_in_working_set(page* working_set, int page_id) {
	for(int i = 0; i < WORKING_SET_LIMIT; i++) {
		if(page_id == working_set[i].page_id)
			return 1;
	}

	return 0;
}

/*
 * Retorna uma pagina se esta presente em uma lista de paginas. Retorna NULL se nao estiver.
 */
page* get_page_from_page_list(page_list* pl, int page_id) {
	page* p = pl->first;
	
	while(p != NULL && p->page_id != page_id) {
		p = p->next;
	}
	
	return p;
}

/**************************** PRINT FUNCTIONS *******************************/

void print_infos() {
	printf(BLUE "\n---print_infos---\n" RESET);
	process* p = PROCESS_LIST.first;
	frame* f = MEMORY.first;
	process* s = SWAP_MEMORY.first;
	
	printf(CYAN"Processes:\n"RESET);
	fprintf(trace, "Processes:\n");
	while(p != NULL) {
		print_process(p);
		p = p->next;
	}
	
	printf(CYAN"Virtual Memory:\n"RESET);
	fprintf(trace, "Virtual Memory:\n");
	while(f != NULL) {
		if(f->page != NULL) {
			printf(GREEN "[%d]: %d(%d)  " RESET, f->frame_id, f->page->page_id, f->page->parent_process_id);
			fprintf(trace, "[%d]: %d(%d)  ", f->frame_id, f->page->page_id, f->page->parent_process_id);
		}
		f = f->next;
	}
	fprintf(trace, "\n-----------------------------------------\n");
	puts("");
	printf(CYAN"SWAP:\n"RESET);
	fprintf(trace, "SWAP:\n");
	while(s != NULL) {
		print_process(s);
		s = s->next;
	}
	fprintf(trace, "\n-----------------------------------------\n");
	printf(BLUE "---ENDprint_infos---\n" RESET);
}

void print_process(process* p) {
	printf(BLUE "---print_process---\n" RESET);

	printf("p->pcb.process_id: %d\n", p->pcb.process_id);
	printf("p->num_requests_done: %d\n", p->num_requests_done);
	printf("working set: ");
	fprintf(trace, "Process ID: %d\n", p->pcb.process_id);
	fprintf(trace, "Requests Done: %d\n", p->num_requests_done);
    fprintf(trace, "Working Set: ");
	for(int i = 0; i < WORKING_SET_LIMIT; i++) { 
		printf("[%d]: %d  ", i, p->working_set[i].page_id);
		fprintf(trace, "%d | ", p->working_set[i].page_id);
	}
	fprintf(trace, "\n-----------------------------------------\n");

	puts("");

	printf(BLUE "---ENDprint_process---\n" RESET);
}

/**************************** PROCESS FUNCTIONS *******************************/

/*
 * Realiza uma requisicao de pagina por um processo.
 *
 * Caso a pagina escolhida ja esteja no working set, atualiza a pagina como mais recentemente visitada.
 * 
 * Caso nao esteja no working set mas o working set estiver cheio, pega a frame ocupada pela pagina referenciada a mais tempo 
 * e a utiliza para a pagina requisitada, removendo a pagina mais antiga do working set e inserindo a nova.
 *
 * Caso nao esteja no working set e o working set nao estiver cheio, a pagina podera ser imediatamente inserida no working set, 
 * porem devera buscar uma nova frame disponivel. Se uma frame disponivel existir, a utiliza para a pagina requisitada. Se 
 * nenhuma existir, o processo que esta a mais tempo na memoria eh removido para a memoria swap e as frames que eram ocupadas 
 * por suas paginas tornam-se disponiveis; uma destas eh utilizada para a pagina requisitada.
 */
void request_page(process* p) {
	
	int random_page_id = random()%NUM_MAX_PAGES + 1;
	page* pg = get_page_from_page_list(&p->page_list, random_page_id);
	
	printf(YELLOW "Processo #%d faz requisicao da pagina #%d.\n" RESET, p->pcb.process_id, random_page_id);
	fprintf(trace, "Process #%d makes Page #%d request\n", p->pcb.process_id ,random_page_id);
	
	if(is_page_in_working_set(p->working_set, random_page_id)) {
		printf(YELLOW "Pagina #%d ja esta presente no working set do processo #%d\n" RESET, random_page_id, p->pcb.process_id);
		//fprintf(trace, "Pagina #%d ja esta presente no working set do processo #%d\n", random_page_id, p->pcb.process_id);
		l_shift(p->working_set, random_page_id);
	}
	else {
		if(is_working_set_full(p)) {
			int oldest_page_id = p->working_set[0].page_id;
			frame* f = get_frame_by_process_and_page(&MEMORY, oldest_page_id, p->pcb.process_id);			// these 3 lines substitute the page in the frame
			p->pagetable[oldest_page_id-1][0] = 0;
			if(f != NULL) {
				printf(YELLOW "Frame #%d, ocupada pela pagina #%d, agora esta ocupada pela pagina #%d\n" RESET, f->frame_id, oldest_page_id, pg->page_id);
				f->page = pg;
				p->pagetable[pg->page_id-1][0] = 1;
				p->pagetable[pg->page_id-1][1] = f->frame_id;
			}
			LRU(p, random_page_id);
		}
		else {
			LRU(p, random_page_id);
			if(is_any_frame_available()) {
				frame* f = get_first_available_frame(&MEMORY);
				f->page = pg;
				p->pagetable[pg->page_id-1][0] = 1;
				p->pagetable[pg->page_id-1][1] = f->frame_id;
				printf(YELLOW "Frame #%d foi alocada a pagina #%d\n" RESET, f->frame_id, pg->page_id);
			}
			else {
				
				printf(YELLOW "Todas as frames estao ocupadas\n" RESET);
				fprintf(trace, "All frames are occupied\n");
				process* pr = get_different_process(p, &PROCESS_LIST);
				printf(YELLOW "Processo #%d foi escolhido para ser swaped out\n" RESET, pr->pcb.process_id);
				fprintf(trace, "Process #%d was selected to be swaped out\n", pr->pcb.process_id);				
				for(int i = 0; i < NUM_MAX_PAGES; i++)
					for(int j = 0; j < 2; j++)
						pr->pagetable[i][j] = 0;

				release_process_frames(pr);
				
				frame* f = get_first_available_frame(&MEMORY);
				f->page = pg;
				printf(YELLOW "Frame #%d foi alocada a pagina #%d\n" RESET, f->frame_id, pg->page_id);
				
				swap_out(pr->pcb.process_id);
			}
		}
	}

	p->has_just_requested_page = 1;
	
	p->num_requests_done++;
	if(p->num_requests_done == NUM_MAX_REQUESTS)
		any_process_done = 1;
}

/*
 * Busca um processo em uma lista pelo seu ID. Retorna NULL se o processo nao esta na lista.
 */
process* get_process(process_list* pl, int process_id) {
	process* p = pl->first;
	
	while(p != NULL && p->pcb.process_id != process_id)
		p = p->next;
	
	return p;
}

/*
 * Insere um processo em uma lista de processos.
 */
void insert_process(process_list* pl, process* new) {
	if(pl->size == 0) {
		pl->first = new;
	}
	else {
		pl->last->next = new;
		new->previous = pl->last;
	}
	
	pl->last = new;
	pl->size++;
}

/*
 * Remove um processo de uma lista de processos.
 */
process* remove_process(process_list* pl, int process_id) {
	process* p = get_process(pl, process_id);
	
	if(p == NULL)
		return NULL;
		
	if(p->previous != NULL)
		p->previous->next = p->next;
	if(p->next != NULL)
		p->next->previous = p->previous;
	
	if(p == pl->first)
		pl->first = pl->first->next;
	if(p == pl->last)
		pl->last = pl->last->previous;
	
	pl->size--;
	
	return p;
}

/*
 * Termina um processo.
 * Desocupa todas as frames que estiverem ocupadas por paginas em seu working set. 
 * Remove o processo da lista de processos onde estiver.
 */
void terminate_process(process* p, process_list* pl) {
	for(int i = 0; i < WORKING_SET_LIMIT; i++) {
		if(p->working_set[i].page_id != 0) {
			frame* f = get_frame_by_process_and_page(&MEMORY, p->working_set[i].page_id, p->pcb.process_id);
			if(f != NULL)
				release_frame(f);
		}
	}
	
	remove_process(pl, p->pcb.process_id);
	
	processes_done_counter++;
	printf(RED"Processo #%d foi terminado, agora processes_done_counter: %d\n"RESET, p->pcb.process_id, processes_done_counter);
	fprintf(trace, "Process #%d has been terminated\n", p->pcb.process_id);
}

/*
 * Busca pelos processos (carregados na memoria ou na area de swap) que ja cumpriram seu numero de requisicoes e termina os que
 * encontrar.
 */
void terminate_done_processes() {
	process* p = PROCESS_LIST.first;
	
	while(p != NULL) {
		if(p->num_requests_done == NUM_MAX_REQUESTS) {
			process* aux = p;
			p = p->next;
			terminate_process(aux, &PROCESS_LIST);
		}
		else
			p = p->next;
	}
	
	p = SWAP_MEMORY.first;
	
	while(p != NULL) {
		if(p->num_requests_done == NUM_MAX_REQUESTS) {
			process* aux = p;
			p = p->next;
			terminate_process(aux, &SWAP_MEMORY);
		}
		else
			p = p->next;
	}
	
	any_process_done = 0;
}

/*
 * Retorna um processo que possa ser alvo de uma operacao swap out e que seja diferente daquele dado 
 * como parametro.
 */
process* get_different_process(process* p, process_list* pl) {
	process* aux = pl->first;
	
	printf("Processo fazendo requisicao: %d\n", p->pcb.process_id);
	fprintf(trace, "Process making request: %d\n", p->pcb.process_id);

	while(aux != NULL && aux->pcb.process_id == p->pcb.process_id)
		aux = aux->next;

	return aux;
}

/*
 * Esvazia o working set de um processo. Faz isso indicando que os IDs das paginas dele sao todos iguais a 0.
 */
void reset_working_set(process* p) {
	for(int i = 0; i < WORKING_SET_LIMIT; i++)
		p->working_set[i].page_id = 0;
}

/*
 * As flags que indicam se um processo ja fez uma requisicao neste instante devem ser reiniciadas a cada novo instante.
 */
void reset_process_flags() {
	process* p = PROCESS_LIST.first;
	while(p != NULL) {
		p->has_just_requested_page = 0;
		p = p->next;
	}
	
	p = SWAP_MEMORY.first;
	while(p != NULL) {
		p->has_just_requested_page = 0;
		p = p->next;
	}
}

/*
 * Retorna 1 se a ultima pagina do working set for uma pagina com ID > 0.
 * Retorna 0 se a ultima pagina for uma pagina real da page list do processo.
 * (ID igual a 0 indica que nao eh uma pagina real)
 */
int is_working_set_full(process* p) {
	if(p->working_set[3].page_id != 0)
		return 1;
	return 0;
}

/***************************** UTILITY FUNCTIONS ********************************/

void update_flags() {
	if(instante_tempo_global % 3 == 0) {
		time_to_create_process = 1;
		time_to_request_page = 1;
	}
	else {
		time_to_create_process = 0;
		time_to_request_page = 0;
	}
	reset_process_flags();
}

/***************************** LRU FUNCTIONS ********************************/

void l_shift(page *working_set, int id) {

    int index = 3;
    for(int i = 3; i >= 0; i--) //buscar indice da ultima posicao ocupada (evitar os 0s)
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
        if(p->working_set[i].page_id == pg->page_id) { //pagina ja esta presente no working set
            l_shift(p->working_set, pg->page_id);      //atualiza working set
            return;
        } else if(p->working_set[i].page_id == 0) {    //nova pagina && existe frame vazio no working set
            p->working_set[i].page_id = pg->page_id;   //primeiro frame vazio recebe a pagina
            return;
        }     
    }
    /* pagina nao esta presente e nao ha espaco vazio no working set: */
    p->working_set[0].page_id = pg->page_id;
    l_shift(p->working_set, pg->page_id);
}

/***************************** SWAP FUNCTIONS ********************************/

/*
 * Retira o processo carregado na memoria para a memoria swap. Retira as paginas
 * de seu working set.
 */
void swap_out(int process_id) {
	printf(BLUE"---swap_out---\n"RESET);
	
	process* p = remove_process(&PROCESS_LIST, process_id);
		
	p->previous = NULL;
	p->next = NULL;
	reset_working_set(p);
	insert_process(&SWAP_MEMORY, p);

	printf(BLUE"---ENDswap_out---\n"RESET);
}

/*
 * Carrega o processo da memoria swap para a memoria.
 */
void swap_in(int process_id) {
	printf(BLUE"---swap_in---\n"RESET);

	process* p = remove_process(&SWAP_MEMORY, process_id);
	printf(RED"Processo #%d foi swaped in\n"RESET, p->pcb.process_id);
	fprintf(trace, "Process #%d has been swaped in\n", p->pcb.process_id);
	p->previous = NULL;
	p->next = NULL;
	insert_process(&PROCESS_LIST, p);
	
	printf(BLUE"---ENDswap_in---\n"RESET);
}

/***************************** MAIN ********************************/

int main(int argc, char* argv[]) {

	srand(time(NULL));

	process* p;

	trace = fopen("Trace.txt", "w");

	while(MEMORY.size < NUM_MAX_FRAMES)
		create_frames();

	while(processes_done_counter < NUM_MAX_PROCESSES) {
		update_flags();
		
		if(time_to_request_page) { 
			printf("\n\n===========INSTANTE DE TEMPO: %d===========\n", instante_tempo_global);
			fprintf(trace, "\n\n================= CURRENT TIME UNIT: %d ============== \n", instante_tempo_global);
		}
		if(time_to_create_process && created_processes < NUM_MAX_PROCESSES)
			create_process();
		
		if(time_to_request_page) {
			p = PROCESS_LIST.first;
			while(p != NULL) {
				request_page(p);
				fprintf(trace, "Pagetable:\n"); 
				for(int i = 0; i < NUM_MAX_PAGES; i++) {
					if(p->pagetable[i][0] == 1) 
						fprintf(trace, "Process #%d : Page #%d in Frame #%d\n", p->pcb.process_id, i+1, p->pagetable[i][1]);
				}
				fprintf(trace, "-----------------------------------------\n");
				p = p->next;
			}
			
			p = SWAP_MEMORY.first;
			while(p != NULL) {
				process* aux = p->next;
				if(!p->has_just_requested_page) {
					request_page(p);
					swap_in(p->pcb.process_id);
				}
				p = aux;
			}
		
			print_infos();
		}
		
		instante_tempo_global++;
		
		if(any_process_done) {
			terminate_done_processes();
		}
	}

	fclose(trace);

    return 0;
}

//LER CADA FUNCAO COM ATENCAO E VER SE NAO TEM NENHUM CASO QUE NAO SEJA COBERTO POR ELAS
//POR FUNCTION CALLS DE TODAS AS FUNCS
//FAZER TESTES COM DIFERENTES PARAMETROS
