/* ======================================================================
   SISTEMA INTEGRADO - Componente 1 (Pouso) + Componente 2 (Taxiway)
   ----------------------------------------------------------------------
   Fluxo de integracao pedido:
     1) O aviao solicita pouso -> entra na fila de prioridade (Componente 1),
        exatamente como antes, sem ainda ocupar nenhum taxiway.
     2) Quando o operador vai AUTORIZAR o pouso do aviao de maior prioridade,
        o sistema primeiro consulta o Componente 2 (circuito de taxiway) e
        lista quais nos de entrada estao livres.
     3) Se existir taxiway livre, o operador escolhe o no de entrada e o
        portao de destino; o aviao entao SAI da fila de prioridade e ENTRA
        no circuito de taxiway - so nesse momento o pouso e de fato
        autorizado.
     4) Se NENHUM taxiway estiver disponivel, o pouso NAO e autorizado: o
        aviao permanece na fila de prioridade, aguardando uma proxima
        tentativa.

   As duas estruturas continuam sendo as mesmas dos componentes originais
   (lista encadeada ordenada por prioridade + lista circular duplamente
   encadeada); apenas a camada de menu/logica de negocio passou a
   consultar as duas juntas antes de confirmar a autorizacao.
   ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define CLEAR_COMMAND "cls"
#else
#define CLEAR_COMMAND "clear"
#endif

int LerNumero(const char* mensagem, int minimo, int maximo)
{
    char entrada[100];
    char* fim;
    long valor;

    while (1)
    {
        printf("%s", mensagem);

        if (fgets(entrada, sizeof(entrada), stdin) == NULL)
            continue;

        entrada[strcspn(entrada, "\n")] = '\0';

        valor = strtol(entrada, &fim, 10);

        if (*entrada == '\0' || *fim != '\0')
        {
            printf("Entrada invalida. Digite apenas numeros.\n\n");
            continue;
        }

        if (valor < minimo || valor > maximo)
        {
            printf("Valor invalido - Min %d, Max %d.\n\n", minimo, maximo);
            continue;
        }

        return (int)valor;
    }
}

void LerTexto(const char* mensagem, char texto[], int tamanho)
{
    while (1)
    {
        printf("%s", mensagem);

        if (fgets(texto, tamanho, stdin) == NULL)
            continue;

        if (strchr(texto, '\n') != NULL)
        {
            texto[strcspn(texto, "\n")] = '\0';
            return;
        }

        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        printf("Valor invalido - Max %d caracteres.\n\n", tamanho - 1);
    }
}

/* ======================================================================
   COMPONENTE 1 - Fila de prioridade de pouso
   ====================================================================== */
enum prioridade
{
    Emergencia = 1,
    Comercial,
    Carga
};

typedef struct AviaoFila
{
    char nome[20];
    enum prioridade prioridade;
    int id_aviao;
    struct AviaoFila* prox;
} AviaoFila;

typedef struct FilaPouso
{
    AviaoFila* inicio;
    AviaoFila* fim;
    int tam;
} FilaPouso;

void CriarFila(FilaPouso* fila)
{
    fila->inicio = NULL;
    fila->fim = NULL;
    fila->tam = 0;
}

void SolicitacaoPouso(FilaPouso* fila, char nome[20], int prioridade, int id)
{
    AviaoFila* novo = malloc(sizeof(AviaoFila));
    AviaoFila* atual;

    if (!novo) return;

    strcpy(novo->nome, nome);
    novo->prioridade = prioridade;
    novo->id_aviao = id;
    novo->prox = NULL;

    if (fila->inicio == NULL)
    {
        fila->inicio = novo;
        fila->fim = novo;
        fila->tam++;
        return;
    }

    if (prioridade < fila->inicio->prioridade)
    {
        novo->prox = fila->inicio;
        fila->inicio = novo;
        fila->tam++;
        return;
    }

    atual = fila->inicio;
    while (atual->prox != NULL && atual->prox->prioridade <= prioridade)
        atual = atual->prox;

    novo->prox = atual->prox;
    atual->prox = novo;

    if (novo->prox == NULL)
        fila->fim = novo;

    fila->tam++;
}

/* Remove o inicio (maior prioridade). Quem chama e responsavel por dar free() depois de usar os dados. */
AviaoFila* PermitirPousoPrioritario(FilaPouso* fila)
{
    AviaoFila* remover = NULL;

    if (fila->inicio)
    {
        remover = fila->inicio;
        fila->inicio = remover->prox;
        if (fila->inicio == NULL)
            fila->fim = NULL;
        fila->tam--;
    }
    return remover;
}

void ListarFila(FilaPouso fila)
{
    AviaoFila* aviao = fila.inicio;
    printf("\n\t\tAvioes aguardando pouso: %d\n", fila.tam);
    while (aviao)
    {
        printf("Nome: %s | ID: %d | Prioridade: %d\n", aviao->nome, aviao->id_aviao, aviao->prioridade);
        aviao = aviao->prox;
    }
}

void DestruirFila(FilaPouso* fila)
{
    AviaoFila* atual = fila->inicio;
    AviaoFila* prox;
    while (atual) { prox = atual->prox; free(atual); atual = prox; }
    fila->inicio = NULL;
    fila->fim = NULL;
    fila->tam = 0;
}

/* ======================================================================
   COMPONENTE 2 - Circuito de taxiway (lista circular dupla)
   ====================================================================== */
typedef struct AviaoTaxi AviaoTaxi;

typedef struct NoTaxiway
{
    int id;
    AviaoTaxi* ocupante;
    struct NoTaxiway* prox;
    struct NoTaxiway* ant;
} NoTaxiway;

struct AviaoTaxi
{
    char nome[20];
    int id_aviao;
    int portao_destino;
    NoTaxiway* posicao;
    AviaoTaxi* prox; /* lista dos avioes ativos em solo */
};

typedef struct Circuito
{
    NoTaxiway* referencia;
    AviaoTaxi* avioes_em_solo;
    int quantidade_nos;
} Circuito;

void CriarCircuito(Circuito* circuito, int quantidade_nos)
{
    NoTaxiway* primeiro = NULL;
    NoTaxiway* anterior = NULL;
    NoTaxiway* novo;
    int i;

    circuito->avioes_em_solo = NULL;
    circuito->quantidade_nos = quantidade_nos;

    for (i = 0; i < quantidade_nos; i++)
    {
        novo = malloc(sizeof(NoTaxiway));
        novo->id = i;
        novo->ocupante = NULL;
        novo->prox = NULL;
        novo->ant = NULL;

        if (primeiro == NULL) primeiro = novo;
        else { anterior->prox = novo; novo->ant = anterior; }
        anterior = novo;
    }

    anterior->prox = primeiro;
    primeiro->ant = anterior;
    circuito->referencia = primeiro;
}

NoTaxiway* BuscarNo(Circuito* circuito, int id)
{
    NoTaxiway* atual = circuito->referencia;
    int i;
    for (i = 0; i < circuito->quantidade_nos; i++)
    {
        if (atual->id == id) return atual;
        atual = atual->prox;
    }
    return NULL;
}

int DistanciaAteDestino(NoTaxiway* origem, int destino, int sentido_horario)
{
    NoTaxiway* atual = origem;
    int passos = 0;
    while (atual->id != destino)
    {
        atual = sentido_horario ? atual->prox : atual->ant;
        passos++;
        if (passos > 10000) return -1;
    }
    return passos;
}

int CaminhoLivre(NoTaxiway* origem, int destino, int sentido_horario)
{
    NoTaxiway* atual = sentido_horario ? origem->prox : origem->ant;
    while (1)
    {
        if (atual->ocupante != NULL) return 0;
        if (atual->id == destino) return 1;
        atual = sentido_horario ? atual->prox : atual->ant;
    }
}

/* Lista os nos livres do circuito. Retorna a quantidade de nos livres encontrados. */
int ListarNosLivres(Circuito* circuito)
{
    NoTaxiway* atual = circuito->referencia;
    int i, livres = 0;

    printf("\nNos de taxiway livres:\n");
    for (i = 0; i < circuito->quantidade_nos; i++)
    {
        if (atual->ocupante == NULL)
        {
            printf("  - No %d\n", atual->id);
            livres++;
        }
        atual = atual->prox;
    }
    if (livres == 0)
        printf("  (nenhum no livre no momento)\n");

    return livres;
}

int EntrarNoCircuito(Circuito* circuito, char nome[20], int id_aviao, int no_entrada, int portao_destino)
{
    NoTaxiway* no = BuscarNo(circuito, no_entrada);
    AviaoTaxi* novo;

    if (no == NULL || no->ocupante != NULL)
        return 0;

    novo = malloc(sizeof(AviaoTaxi));
    strcpy(novo->nome, nome);
    novo->id_aviao = id_aviao;
    novo->portao_destino = portao_destino;
    novo->posicao = no;
    novo->prox = circuito->avioes_em_solo;
    circuito->avioes_em_solo = novo;

    no->ocupante = novo;
    return 1;
}

void RemoverDaListaSolo(Circuito* circuito, AviaoTaxi* alvo)
{
    AviaoTaxi* atual = circuito->avioes_em_solo;
    AviaoTaxi* anterior = NULL;

    while (atual)
    {
        if (atual == alvo)
        {
            if (anterior == NULL) circuito->avioes_em_solo = atual->prox;
            else anterior->prox = atual->prox;
            free(atual);
            return;
        }
        anterior = atual;
        atual = atual->prox;
    }
}

#define LOGO_ATRAS 2

void MoverAvioes(Circuito* circuito)
{
    AviaoTaxi* atual = circuito->avioes_em_solo;
    AviaoTaxi* proximo_da_lista;

    while (atual)
    {
        proximo_da_lista = atual->prox;

        if (atual->posicao->id == atual->portao_destino)
        {
            printf("Aviao %s (ID %d) liberou o taxiway e atracou no portao %d!\n",
                   atual->nome, atual->id_aviao, atual->portao_destino);
            atual->posicao->ocupante = NULL;
            RemoverDaListaSolo(circuito, atual);
        }
        else
        {
            int dist_anti = DistanciaAteDestino(atual->posicao, atual->portao_destino, 0);
            int sentido_horario = 1;

            if (dist_anti >= 1 && dist_anti <= LOGO_ATRAS &&
                CaminhoLivre(atual->posicao, atual->portao_destino, 0))
                sentido_horario = 0;

            NoTaxiway* proximo_no = sentido_horario ? atual->posicao->prox : atual->posicao->ant;

            if (proximo_no->ocupante == NULL)
            {
                atual->posicao->ocupante = NULL;
                proximo_no->ocupante = atual;
                atual->posicao = proximo_no;
            }
        }
        atual = proximo_da_lista;
    }
}

void ListarCircuito(Circuito* circuito)
{
    NoTaxiway* atual = circuito->referencia;
    int i;
    printf("\n\t\tCircuito de taxiways (%d nos):\n", circuito->quantidade_nos);
    for (i = 0; i < circuito->quantidade_nos; i++)
    {
        if (atual->ocupante)
            printf("No %d: aviao %s (ID %d), destino portao %d\n",
                   atual->id, atual->ocupante->nome, atual->ocupante->id_aviao, atual->ocupante->portao_destino);
        else
            printf("No %d: livre\n", atual->id);
        atual = atual->prox;
    }
}

void DestruirCircuito(Circuito* circuito)
{
    NoTaxiway* atual = circuito->referencia;
    NoTaxiway* inicio = circuito->referencia;
    NoTaxiway* proximo;

    while (circuito->avioes_em_solo)
        RemoverDaListaSolo(circuito, circuito->avioes_em_solo);

    if (atual == NULL) return;
    do 
    { 
        proximo = atual->prox;
        free(atual);
        atual = proximo; 
    } while (atual != inicio);
}

/* ======================================================================
   INTEGRACAO - Autorizacao de pouso condicionada a taxiway disponivel
   ====================================================================== */
void AutorizarPousoComTaxiway(FilaPouso* fila, Circuito* circuito)
{
    AviaoFila* proximo;
    AviaoFila* removido;
    int livres, no_entrada, portao_destino;
    NoTaxiway* no;

    if (fila->inicio == NULL)
    {
        printf("Nenhum aviao na fila aguardando pouso.\n");
        return;
    }

    /* 1) Verifica quem seria o proximo a pousar, SEM removê-lo ainda da fila */
    proximo = fila->inicio;
    printf("\nProximo da fila: %s (ID %d, prioridade %d)\n",
           proximo->nome, proximo->id_aviao, proximo->prioridade);

    /* 2) Consulta o Componente 2: quais taxiways estao disponiveis agora */
    livres = ListarNosLivres(circuito);

    if (livres == 0)
    {
        printf("Pouso NAO autorizado: nenhum taxiway disponivel no momento.\n"
               "Aviao %s permanece na fila de espera.\n", proximo->nome);
        return; /* aviao NAO sai da fila */
    }

    /* 3) Operador escolhe o no de entrada (dentre os livres) e o portao de destino */
    no_entrada = LerNumero("Escolha o no de taxiway para o pouso: ", 0, circuito->quantidade_nos - 1);
    no = BuscarNo(circuito, no_entrada);

    if (no == NULL || no->ocupante != NULL)
    {
        printf("No escolhido invalido ou ja ocupado. Pouso NAO autorizado; aviao permanece na fila.\n");
        return; /* aviao NAO sai da fila */
    }

    portao_destino = LerNumero("Portao de destino do aviao no taxiway: ", 0, circuito->quantidade_nos - 1);

    /* 4) So agora, com taxiway confirmado, o aviao sai da fila de prioridade... */
    removido = PermitirPousoPrioritario(fila);

    /* ...e entra fisicamente no circuito de taxiway */
    EntrarNoCircuito(circuito, removido->nome, removido->id_aviao, no_entrada, portao_destino);

    printf("\nPOUSO AUTORIZADO! Aviao %s (ID %d) pousou, entrou no taxiway pelo No %d e seguira ate o portao %d.\n",
           removido->nome, removido->id_aviao, no_entrada, portao_destino);

    free(removido);
}

/* ======================================================================
   MENU PRINCIPAL
   ====================================================================== */
void SistemaIntegrado()
{
    system(CLEAR_COMMAND);

    FilaPouso fila;
    Circuito circuito;
    int opcao, quantidade_nos, id, prioridade;
    char nome[20];

    CriarFila(&fila);

    quantidade_nos = LerNumero("Quantidade de nos do circuito de taxiway: ", 3, 50);
    CriarCircuito(&circuito, quantidade_nos);

    do
    {
        opcao = LerNumero(
            "\n\t0 - Sair\n\t"
            "1 - Aviao solicita pouso (entra na fila de prioridade)\n\t"
            "2 - Autorizar pouso prioritario (verifica taxiway antes)\n\t"
            "3 - Avancar passo de simulacao do taxiway\n\t"
            "4 - Listar fila de pouso\n\t"
            "5 - Listar circuito de taxiway\n\t"
            "Selecione: ", 0, 5);

        switch (opcao)
        {
        case 1:
            system(CLEAR_COMMAND);
            LerTexto("Nome da aeronave: ", nome, 19);
            id = LerNumero("ID da aeronave: ", 1, 1000);
            prioridade = LerNumero("Prioridade (1 Emergencia, 2 Comercial, 3 Carga): ", 1, 3);
            SolicitacaoPouso(&fila, nome, prioridade, id);
            break;
        case 2:
            system(CLEAR_COMMAND);
            AutorizarPousoComTaxiway(&fila, &circuito);
            break;
        case 3:
            system(CLEAR_COMMAND);
            MoverAvioes(&circuito);
            break;
        case 4:
            system(CLEAR_COMMAND);
            ListarFila(fila);
            break;
        case 5:
            system(CLEAR_COMMAND);
            ListarCircuito(&circuito);
            break;
        default:
            if (opcao != 0)
                printf("Opcao invalida!\n");
        }
    } while (opcao != 0);

    DestruirFila(&fila);
    DestruirCircuito(&circuito);
}

int main()
{
    system(CLEAR_COMMAND);
    SistemaIntegrado();
    return 0;
}
