#include <stdio.h>

/*
 * paginas virtuais por processo/thread: 50
 * working set limit: 4
 */

/*
 * recebe o array de controle e a pagina 
 * faz um left shift a partir da posicao da pagina no array de controle
 */
void l_shift(int array[], int value) {

    int index = 3;
    for(int i = 3; i >= 0; i--)             //buscar indice da ultima posicao ocupada (evitar os 0s)
        if(array[i] == 0)
            index = i-1;

    for(int i = 0; i < 4; i++) {

        if(array[i] == value) {

            int temp = array[i];
            for(int j = i; j < index; j++)
                array[j] = array[j+1];

            array[index] = temp;

        }

    }

}

void push(int array[], int value) {

    for(int i = 0; i < 4; i++)
        if(array[i] == 0)
            array[i] = value;

}

/* guarda as 4 paginas atuais */
int working_set[4] = {0};

/* array de controle - guarda as paginas na ordem menos recente -> mais recente */
int indexes[4] = {0};

/*
 * recebe o numero da pagina e o working set do processo/thread
 * se houver espaço, adiciona a pagina ao fim do working set
 * caso contrario, remove a pagina utilizada a mais tempo (least recently used)
 */
void LRU(int page, int working_set[], int indexes[]) {

    int oldest;                                         //indice da pagina referenciada a mais tempo

    for(int i = 0; i < 4; i++) {

        if(working_set[i] == page) {                    //pagina ja esta presente no working set

            l_shift(&indexes, page);                    //atualiza array de controle
            return;

        } else if(working_set[i] == 0) {                //nova pagina && existe frame vazio no working set

            working_set[i] = page;                      //primeiro frame vazio recebe a pagina
            push(&indexes, page);                       //array de controle recebe pagina na primeira posicao vazia
            /* incrementar page fault */
            return;

        }                           

    }

    /* pagina nao esta presente e nao ha espaco vazio no working set: */
    oldest = indexes[0];
    working_set[oldest] = page;
    indexes[0] = page;
    l_shift(&indexes, page);
    /* incrementar page fault */

}

int main(int argc, char* argv[]) {

    return 0;

}

/*
 * consideracoes:
 *      working_set e indexes fazem parte da estrutura processo/thread
 *      indexes[0] guarda a pagina referenciada a mais tempo sempre
 *      adicionar contador page fault
 */