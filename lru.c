#include <stdio.h>

/*
 * paginas virtuais por processo/thread: 50
 * working set limit: 4
 */

/* guarda as 4 paginas atuais */
int working_set[4] = {0};

/* guarda os indices das paginas na ordem mais recente -> menos recente */
int indexes[4] = {0};

/*
 * recebe o numero da pagina e o working set do processo/thread
 * se houver espaço, adiciona a pagina ao fim do working set
 * caso contrario, remove a pagina utilizada a mais tempo (least recently used)
 */
void LRU(int page, int working_set[], int indexes[]) {

    int oldest;

    for(int i = 0; i < 4; i++) {

        if(working_set[i] == 0) {               //existe frame vazio no working set

            working_set[i] = page;              //primeiro frame vazio recebe a pagina
            /* salvar indice */
            /* incrementar page fault */
            return;

        } else if(working_set[i] == page) {     //pagina ja esta presente no working set

            /* salvar indice */
            return;

        }                              


    }

    /* pagina nao esta presente e nao ha espaco vazio no working set: */
    oldest = indexes[3];
    working_set[oldest] = page;
    /* salvar indice */
    /* incrementar page fault */

}

int main(int argc, char* argv[]) {

    return 0;

}

/*
 * consideracoes:
 *      working_set e indexes fazem parte da estrutura processo/thread
 *      indexes[3] guarda a pagina referenciada a mais tempo sempre
 *      adicionar contador page fault
 */