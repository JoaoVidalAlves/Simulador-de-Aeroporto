/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   COMPONENTE 2 - Circulacao e Gerenciamento de Solo (Taxiways)
   ----------------------------------------------------------------------
   Estrutura escolhida: LISTA CIRCULAR DUPLAMENTE ENCADEADA.
   Cada no representa uma posicao fisica do circuito de taxiway que
   contorna o terminal. E circular porque o circuito nao tem inicio/fim
   real (rotatoria); e dupla porque o sistema precisa navegar tanto em
   sentido horario (padrao) quanto anti-horario (quando o portao esta
   logo atras e o caminho esta livre).

   Cada aviao em solo guarda apenas um ponteiro para o no onde esta -
   entrar/sair do circuito NUNCA realoca ou quebra a lista, apenas marca
   o campo "ocupante" do no como livre/ocupado, entao a circulacao dos
   demais avioes nunca e interrompida.
   <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

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

typedef struct Aviao Aviao;

typedef struct NoTaxiway
{
    int id;                     /* posicao no circuito, 0..N-1 */
    Aviao* ocupante;             /* aviao presente neste no, ou NULL */
    struct NoTaxiway* prox;      /* proximo no em sentido horario */
    struct NoTaxiway* ant;       /* proximo no em sentido anti-horario */
} NoTaxiway;

struct Aviao
{
    char nome[20];
    int id_aviao;
    int portao_destino;         /* id do no/portao onde deve estacionar */
    NoTaxiway* posicao;          /* no atual do circuito */
    Aviao* prox;                 /* lista simples dos avioes ativos em solo */
};

typedef struct Circuito
{
    NoTaxiway* referencia;       /* um no qualquer, usado como ponto de partida das buscas */
    Aviao* avioes_em_solo;       /* lista dos avioes atualmente circulando */
    int quantidade_nos;
} Circuito;

void CriarCircuito(Circuito* circuito, int quantidade_nos)
{
    NoTaxiway* primeiro = NULL;
    NoTaxiway* anterior = NULL;
    NoTaxiway* novo = NULL;
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

        if (primeiro == NULL)
        {
            primeiro = novo;
        }
        else
        {
            anterior->prox = novo;
            novo->ant = anterior;
        }
        anterior = novo;
    }

    /* fecha o circulo */
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
        if (atual->id == id)
            return atual;
        atual = atual->prox;
    }
    return NULL;
}

/* Distancia (numero de passos) de "origem" ate o no de id "destino", andando em um sentido */
int DistanciaAteDestino(NoTaxiway* origem, int destino, int sentido_horario)
{
    NoTaxiway* atual = origem;
    int passos = 0;

    while (atual->id != destino)
    {
        atual = sentido_horario ? atual->prox : atual->ant;
        passos++;
        if (passos > 10000) return -1; /* seguranca contra circuito invalido */
    }
    return passos;
}

/* Verifica se o caminho entre origem (exclusive) e destino (inclusive) esta livre de outros avioes */
int CaminhoLivre(NoTaxiway* origem, int destino, int sentido_horario)
{
    NoTaxiway* atual = sentido_horario ? origem->prox : origem->ant;

    while (1)
    {
        if (atual->ocupante != NULL)
            return 0;
        if (atual->id == destino)
            return 1;
        atual = sentido_horario ? atual->prox : atual->ant;
    }
}

/* Aviao entra no circuito por um no de entrada (pos-pouso). Retorna 1 se conseguiu, 0 se ponto ocupado */
int EntrarNoCircuito(Circuito* circuito, char nome[20], int id_aviao, int no_entrada, int portao_destino)
{
    NoTaxiway* no = BuscarNo(circuito, no_entrada);
    Aviao* novo;

    if (no == NULL || no->ocupante != NULL)
        return 0;

    novo = malloc(sizeof(Aviao));
    strcpy(novo->nome, nome);
    novo->id_aviao = id_aviao;
    novo->portao_destino = portao_destino;
    novo->posicao = no;
    novo->prox = circuito->avioes_em_solo;
    circuito->avioes_em_solo = novo;

    no->ocupante = novo;
    return 1;
}

/* Remove o aviao da lista de avioes em solo (usado quando ele estaciona no portao) */
void RemoverDaListaSolo(Circuito* circuito, Aviao* alvo)
{
    Aviao* atual = circuito->avioes_em_solo;
    Aviao* anterior = NULL;

    while (atual)
    {
        if (atual == alvo)
        {
            if (anterior == NULL)
                circuito->avioes_em_solo = atual->prox;
            else
                anterior->prox = atual->prox;
            free(atual);
            return;
        }
        anterior = atual;
        atual = atual->prox;
    }
}

/*
   Executa um "passo" (tick) de simulacao para todos os avioes:
   - Se ja esta no portao destino, libera o portao (estaciona e sai do circuito).
   - Senao, decide o sentido: horario por padrao; anti-horario somente se o
     portao estiver a poucos nos de distancia (LOGO ATRAS) E o caminho estiver
     livre. Se o proximo no do sentido escolhido estiver ocupado, o aviao
     aguarda nesta rodada (isso cria naturalmente a alternancia de fluxo
     quando dois avioes se cruzam em sentidos opostos no mesmo trecho).
*/
#define LOGO_ATRAS 2 /* distancia (em nos) considerada "logo atras" da posicao atual */

void MoverAvioes(Circuito* circuito)
{
    Aviao* atual = circuito->avioes_em_solo;
    Aviao* proximo_da_lista;

    while (atual)
    {
        proximo_da_lista = atual->prox; /* guarda antes, pois "atual" pode ser removido/liberado */

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
            {
                sentido_horario = 0;
            }

            NoTaxiway* proximo_no = sentido_horario ? atual->posicao->prox : atual->posicao->ant;

            if (proximo_no->ocupante == NULL)
            {
                atual->posicao->ocupante = NULL;
                proximo_no->ocupante = atual;
                atual->posicao = proximo_no;
            }
            /* senao: proximo no ocupado -> aviao aguarda esta rodada (evita colisao) */
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
                   atual->id, atual->ocupante->nome, atual->ocupante->id_aviao,
                   atual->ocupante->portao_destino);
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

void SistemaTaxiway()
{
    system(CLEAR_COMMAND);

    Circuito circuito;
    int opcao, quantidade_nos, no_entrada, portao_destino, id;
    char nome[20];
    int criado = 0;

    quantidade_nos = LerNumero("Quantidade de nos do circuito de taxiway: ", 3, 50);
    CriarCircuito(&circuito, quantidade_nos);
    criado = 1;

    do
    {
        opcao = LerNumero(
            "\n\t0 - Sair\n\t"
            "1 - Entrada de aviao no circuito (pos-pouso)\n\t"
            "2 - Avancar um passo de simulacao (todos os avioes)\n\t"
            "3 - Listar circuito\n\t"
            "Selecione: ", 0, 3);

        switch (opcao)
        {
        case 1:
            LerTexto("Nome da aeronave: ", nome, 19);
            id = LerNumero("ID da aeronave: ", 1, 1000);
            no_entrada = LerNumero("No de entrada no circuito: ", 0, quantidade_nos - 1);
            portao_destino = LerNumero("No do portao de destino: ", 0, quantidade_nos - 1);

            if (!EntrarNoCircuito(&circuito, nome, id, no_entrada, portao_destino))
                printf("No de entrada ocupado, tente outro ou aguarde.\n");
            break;
        case 2:
            MoverAvioes(&circuito);
            break;
        case 3:
            ListarCircuito(&circuito);
            break;
        default:
            if (opcao != 0)
                printf("Opcao invalida!\n");
        }
    } while (opcao != 0);

    if (criado)
        DestruirCircuito(&circuito);
}

int main()
{
    system(CLEAR_COMMAND);
    SistemaTaxiway();
    return 0;
}
