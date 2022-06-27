# Gerenciador-MV
Simulador de um gerenciador de memória virtual utilizando LRU

Premissas a serem consideradas pelo grupo para o Desenvolvimento do Simulador:

a. Gerenciamento de Memória Virtual

    Simule a criação de um gerenciador de memória, através do seu algoritmo de
substituição de páginas LRU, onde o ambiente possui as seguintes características:

    Cada thread de usuário possui um working set limit de até 4 (quatro) frames;

    A memória é limitada em 64 frames dedicados para programas de usuário;

b. Testes e execuções do programa

    Implementar o algoritmo de substituição de páginas LRU.

    Os testes devem ser realizados da seguinte forma:
    
        Cada processo é criado a cada 3 segundos;

        Cada processo criado solicita a alocação de uma página aleatória na memória a
    cada 3 segundos;

        A cada solicitação de página o gerenciador da MV tem que apresentar a tabela
    de páginas virtuais do processo solicitante;

        Testar para 20 threads com 50 páginas virtuais cada.
        
        Um critério para que o processo seja “retirado” da memória (swap out) pode ser
    quando ele for o processo mais antigo. E ele deve retornar a MP quando voltar
    a ser executado.

    Não deixe de elaborar uma forma de monitoramento (trace) das ocorrências de criação
dos threads, alocação de memória real de acordo com a solicitação das páginas, lista
de substituição de páginas (LRU), swapping e execução dos diversos processos
concorrentes, através de um esquema de visualização das informações em memória.