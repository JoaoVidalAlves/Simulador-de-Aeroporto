/* ======================================================================
   SISTEMA DE CONTROLE DE TRAFEGO AEREO E PISTAS DE POUSO
   ----------------------------------------------------------------------
   Integra os TRES componentes exigidos pelo enunciado, usando em cada um
   APENAS os requisitos obrigatorios (nada de politicas extras, pouso
   forcado de emergencia sem taxiway, temporizador real, geracao
   aleatoria de dados etc. - tudo isso ficou de fora de proposito).

   COMPONENTE 1 - Aproximacao e Autorizacao de Pouso
     Lista encadeada simples, ordenada por prioridade na insercao.

   COMPONENTE 2 - Circulacao e Gerenciamento de Solo (Taxiways)
     Lista circular duplamente encadeada. Um aviao so sai da fila do
     Componente 1 quando ha taxiway livre (integracao 1<->2). O
     deslocamento respeita sentido horario padrao / anti-horario quando
     o portao esta logo atras e o caminho livre, e resolve impasses
     fazendo o aviao aguardar quando o proximo no esta ocupado.

   COMPONENTE 3 - Despacho de Porao, Cadastro Previo e Inspecao
     Uma lista (manifesto) + uma pilha (porao fisico, LIFO) POR AVIAO,
     identificadas pelo id_aviao. O descarregamento de um aviao so fica
     liberado depois que ele efetivamente estaciona no portao no
     Componente 2 (integracao 2<->3), conforme o enunciado: "ao
     estacionar no portao designado, inicia-se o processo de
     descarregamento".
   ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define CLEAR_COMMAND "cls"
#else
#define CLEAR_COMMAND "clear"
#endif

/* ---------------------------------------------------------------------
   Entrada de dados auxiliares (usadas pelos 3 componentes)
   --------------------------------------------------------------------- */
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

float LerFloat(const char* mensagem, float minimo, float maximo)
{
    char entrada[100];
    char* fim;
    float valor;

    while (1)
    {
        printf("%s", mensagem);
        if (fgets(entrada, sizeof(entrada), stdin) == NULL)
            continue;
        entrada[strcspn(entrada, "\n")] = '\0';
        valor = strtof(entrada, &fim);
        if (*entrada == '\0' || *fim != '\0')
        {
            printf("Entrada invalida. Digite um numero (ex: 12.5).\n\n");
            continue;
        }
        if (valor < minimo || valor > maximo)
        {
            printf("Valor invalido - Min %.2f, Max %.2f.\n\n", minimo, maximo);
            continue;
        }
        return valor;
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

/* Insere mantendo a lista ordenada por prioridade crescente (1 = mais urgente) */
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

/* Remove o inicio (maior prioridade). Quem chama deve dar free() apos usar os dados. */
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
   COMPONENTE 3 - Despacho de porao (uma pilha + um manifesto POR AVIAO)
   Definido antes do Componente 2 porque o Componente 2 (liberacao de
   portao) precisa avisar o Componente 3 que o aviao ja pode descarregar.
   ====================================================================== */
enum StatusItem
{
    Cadastrado = 0,
    Embarcado,
    Descarregado,
    Isolado
};

typedef struct ItemManifesto
{
    int id;
    char dono[30];
    float peso;
    enum StatusItem status;
    struct ItemManifesto* prox;
} ItemManifesto;

typedef struct NoPorao
{
    int id;
    char dono[30];
    float peso;
    struct NoPorao* prox;
} NoPorao;

/* Registro de bagagem de UM aviao especifico */
typedef struct BagagemAviao
{
    int id_aviao;
    int estacionado; /* 0 = ainda em taxiway, 1 = ja parou no portao (libera descarregamento) */
    ItemManifesto* manifesto;
    ItemManifesto* fim_manifesto;
    NoPorao* topo;
    struct BagagemAviao* prox;
} BagagemAviao;

typedef struct SistemaBagagemGeral
{
    BagagemAviao* avioes; /* lista de registros de bagagem, um por aviao */
} SistemaBagagemGeral;

void CriarSistemaBagagem(SistemaBagagemGeral* sistema)
{
    sistema->avioes = NULL;
}

BagagemAviao* BuscarBagagem(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* atual = sistema->avioes;
    while (atual)
    {
        if (atual->id_aviao == id_aviao) return atual;
        atual = atual->prox;
    }
    return NULL;
}

BagagemAviao* BuscarOuCriarBagagem(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* achado = BuscarBagagem(sistema, id_aviao);
    if (achado) return achado;

    BagagemAviao* novo = malloc(sizeof(BagagemAviao));
    novo->id_aviao = id_aviao;
    novo->estacionado = 0;
    novo->manifesto = NULL;
    novo->fim_manifesto = NULL;
    novo->topo = NULL;
    novo->prox = sistema->avioes;
    sistema->avioes = novo;
    return novo;
}

/* Chamado pelo Componente 2 quando o aviao efetivamente estaciona no portao */
void MarcarEstacionado(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* b = BuscarOuCriarBagagem(sistema, id_aviao);
    b->estacionado = 1;
}

/* Cadastro previo (antes da decolagem na origem) */
void CadastrarItem(SistemaBagagemGeral* sistema, int id_aviao, int id_item, char dono[30], float peso)
{
    BagagemAviao* b = BuscarOuCriarBagagem(sistema, id_aviao);
    ItemManifesto* novo = malloc(sizeof(ItemManifesto));
    novo->id = id_item;
    strcpy(novo->dono, dono);
    novo->peso = peso;
    novo->status = Cadastrado;
    novo->prox = NULL;

    if (b->manifesto == NULL) b->manifesto = novo;
    else b->fim_manifesto->prox = novo;
    b->fim_manifesto = novo;
}

ItemManifesto* ConsultarItem(SistemaBagagemGeral* sistema, int id_aviao, int id_item)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    if (!b) return NULL;
    ItemManifesto* atual = b->manifesto;
    while (atual)
    {
        if (atual->id == id_item) return atual;
        atual = atual->prox;
    }
    return NULL;
}

int RemoverAdministrativo(SistemaBagagemGeral* sistema, int id_aviao, int id_item)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    if (!b) return 0;

    ItemManifesto* atual = b->manifesto;
    ItemManifesto* anterior = NULL;

    while (atual)
    {
        if (atual->id == id_item)
        {
            if (atual->status != Cadastrado)
                return 0; /* ja embarcado, nao pode remover administrativamente */

            if (anterior == NULL) b->manifesto = atual->prox;
            else anterior->prox = atual->prox;
            if (atual == b->fim_manifesto) b->fim_manifesto = anterior;
            free(atual);
            return 1;
        }
        anterior = atual;
        atual = atual->prox;
    }
    return 0;
}

/* Empilha no porao todos os itens ainda "Cadastrado" (embarque fisico) */
void EmbarcarItens(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    if (!b) { printf("Nenhum item cadastrado para esse aviao.\n"); return; }

    ItemManifesto* atual = b->manifesto;
    while (atual)
    {
        if (atual->status == Cadastrado)
        {
            NoPorao* novo = malloc(sizeof(NoPorao));
            novo->id = atual->id;
            strcpy(novo->dono, atual->dono);
            novo->peso = atual->peso;
            novo->prox = b->topo;
            b->topo = novo;
            atual->status = Embarcado;
        }
        atual = atual->prox;
    }
    printf("Itens cadastrados do aviao %d foram embarcados no porao.\n", id_aviao);
}

/* Retorna: 1 = ok, 0 = alerta de inconformidade, -1 = porao vazio, -2 = aviao ainda nao estacionou, -3 = aviao sem registro */
int DescarregarProximo(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    if (!b) return -3;
    if (!b->estacionado) return -2;

    NoPorao* topo = b->topo;
    if (topo == NULL) return -1;

    ItemManifesto* referencia = ConsultarItem(sistema, id_aviao, topo->id);

    if (referencia == NULL)
    {
        printf("*** ALERTA: item ID %d retirado do porao NAO CONSTA no cadastro previo! ***\n", topo->id);
        b->topo = topo->prox;
        free(topo);
        return 0;
    }

    if (strcmp(referencia->dono, topo->dono) != 0 || referencia->peso != topo->peso)
    {
        printf("*** ALERTA: item ID %d com divergencia de dados (dono/peso)! ***\n", topo->id);
        b->topo = topo->prox;
        free(topo);
        return 0;
    }

    printf("Item ID %d (%s, %.2fkg) descarregado e conferido com sucesso.\n", topo->id, topo->dono, topo->peso);
    referencia->status = Descarregado;
    b->topo = topo->prox;
    free(topo);
    return 1;
}

/* Procedimento de interceptacao de emergencia: A) remove ate o alvo (pilha auxiliar),
   B) isola o alvo, C) restaura a ordem original dos demais. */
void InspecaoEmergencia(SistemaBagagemGeral* sistema, int id_aviao, int id_alvo)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    if (!b) { printf("Aviao sem registro de bagagem.\n"); return; }

    NoPorao* auxiliar = NULL;
    NoPorao* temp;
    int encontrado = 0;

    while (b->topo != NULL)
    {
        temp = b->topo;
        b->topo = temp->prox;

        if (temp->id == id_alvo)
        {
            ItemManifesto* referencia = ConsultarItem(sistema, id_aviao, temp->id);
            if (referencia) referencia->status = Isolado;
            printf("Bagagem ID %d ISOLADA para inspecao (dono: %s, peso: %.2fkg).\n",
                   temp->id, temp->dono, temp->peso);
            free(temp);
            encontrado = 1;
            break;
        }

        temp->prox = auxiliar;
        auxiliar = temp;
    }

    if (!encontrado)
        printf("Bagagem ID %d nao encontrada no porao deste aviao.\n", id_alvo);

    while (auxiliar != NULL)
    {
        temp = auxiliar;
        auxiliar = temp->prox;
        temp->prox = b->topo;
        b->topo = temp;
    }
}

void ListarManifesto(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    const char* nomes[] = {"Cadastrado", "Embarcado", "Descarregado", "Isolado"};
    printf("\n\t\tManifesto do aviao %d:\n", id_aviao);
    if (!b || !b->manifesto) { printf("(vazio)\n"); return; }
    ItemManifesto* atual = b->manifesto;
    while (atual)
    {
        printf("ID: %d | Dono: %s | Peso: %.2fkg | Status: %s\n",
               atual->id, atual->dono, atual->peso, nomes[atual->status]);
        atual = atual->prox;
    }
}

void ListarPorao(SistemaBagagemGeral* sistema, int id_aviao)
{
    BagagemAviao* b = BuscarBagagem(sistema, id_aviao);
    printf("\n\t\tPorao do aviao %d (topo -> fundo):\n", id_aviao);
    if (!b || !b->topo) { printf("(vazio)\n"); return; }
    NoPorao* atual = b->topo;
    while (atual)
    {
        printf("ID: %d | Dono: %s | Peso: %.2fkg\n", atual->id, atual->dono, atual->peso);
        atual = atual->prox;
    }
}

void DestruirSistemaBagagem(SistemaBagagemGeral* sistema)
{
    BagagemAviao* b = sistema->avioes;
    BagagemAviao* prox_b;

    while (b)
    {
        ItemManifesto* m = b->manifesto;
        ItemManifesto* pm;
        while (m) { pm = m->prox; free(m); m = pm; }

        NoPorao* p = b->topo;
        NoPorao* pp;
        while (p) { pp = p->prox; free(p); p = pp; }

        prox_b = b->prox;
        free(b);
        b = prox_b;
    }
    sistema->avioes = NULL;
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
    AviaoTaxi* prox;
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

/* Lista os nos livres do circuito. Retorna a quantidade de nos livres. */
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

#define LOGO_ATRAS 2 /* distancia (em nos) considerada "logo atras" da posicao atual */

/* Passo de simulacao. Recebe o sistema de bagagem para avisar o Componente 3
   quando um aviao efetivamente estaciona (integracao 2 <-> 3). */
void MoverAvioes(Circuito* circuito, SistemaBagagemGeral* bagagens)
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

            MarcarEstacionado(bagagens, atual->id_aviao); /* -> Componente 3 */

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
            /* senao: proximo no ocupado -> aviao aguarda esta rodada (resolve impasse) */
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
    do { proximo = atual->prox; free(atual); atual = proximo; } while (atual != inicio);
}

/* ======================================================================
   INTEGRACAO 1 <-> 2: autorizacao de pouso condicionada a taxiway livre
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

    proximo = fila->inicio;
    printf("\nProximo da fila: %s (ID %d, prioridade %d)\n",
           proximo->nome, proximo->id_aviao, proximo->prioridade);

    livres = ListarNosLivres(circuito);

    if (livres == 0)
    {
        printf("Pouso NAO autorizado: nenhum taxiway disponivel no momento.\n"
               "Aviao %s permanece na fila de espera.\n", proximo->nome);
        return;
    }

    no_entrada = LerNumero("Escolha o no de taxiway para o pouso: ", 0, circuito->quantidade_nos - 1);
    no = BuscarNo(circuito, no_entrada);

    if (no == NULL || no->ocupante != NULL)
    {
        printf("No escolhido invalido ou ja ocupado. Pouso NAO autorizado; aviao permanece na fila.\n");
        return;
    }

    portao_destino = LerNumero("Portao de destino do aviao no taxiway: ", 0, circuito->quantidade_nos - 1);

    removido = PermitirPousoPrioritario(fila);
    EntrarNoCircuito(circuito, removido->nome, removido->id_aviao, no_entrada, portao_destino);

    printf("\nPOUSO AUTORIZADO! Aviao %s (ID %d) pousou, entrou no taxiway pelo No %d e seguira ate o portao %d.\n",
           removido->nome, removido->id_aviao, no_entrada, portao_destino);

    free(removido);
}

/* ======================================================================
   MENU PRINCIPAL
   ====================================================================== */
void SistemaAeroporto()
{
    system(CLEAR_COMMAND);

    FilaPouso fila;
    Circuito circuito;
    SistemaBagagemGeral bagagens;

    int opcao, quantidade_nos, id_aviao, id_item, prioridade;
    char nome[20], dono[30];
    float peso;

    CriarFila(&fila);
    CriarSistemaBagagem(&bagagens);

    quantidade_nos = LerNumero("Quantidade de nos do circuito de taxiway: ", 3, 50);
    CriarCircuito(&circuito, quantidade_nos);

    do
    {
        opcao = LerNumero(
            "\n\t0  - Sair\n\t"
            "--- Componente 1 (pouso) ---\n\t"
            "1  - Aviao solicita pouso\n\t"
            "--- Integracao 1+2 ---\n\t"
            "2  - Autorizar pouso prioritario (verifica taxiway)\n\t"
            "3  - Avancar passo de simulacao do taxiway\n\t"
            "4  - Listar fila de pouso\n\t"
            "5  - Listar circuito de taxiway\n\t"
            "--- Componente 3 (bagagem, por aviao) ---\n\t"
            "6  - Cadastrar item no manifesto (pre-voo)\n\t"
            "7  - Consultar item no manifesto\n\t"
            "8  - Remocao administrativa\n\t"
            "9  - Embarcar itens cadastrados (empilhar no porao)\n\t"
            "10 - Descarregar proximo item do porao (double-check)\n\t"
            "11 - Inspecao de emergencia (isolar item)\n\t"
            "12 - Listar manifesto\n\t"
            "13 - Listar porao\n\t"
            "Selecione: ", 0, 13);

        switch (opcao)
        {
        case 1:
            LerTexto("Nome da aeronave: ", nome, 19);
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            prioridade = LerNumero("Prioridade (1 Emergencia, 2 Comercial, 3 Carga): ", 1, 3);
            SolicitacaoPouso(&fila, nome, prioridade, id_aviao);
            break;
        case 2:
            AutorizarPousoComTaxiway(&fila, &circuito);
            break;
        case 3:
            MoverAvioes(&circuito, &bagagens);
            break;
        case 4:
            ListarFila(fila);
            break;
        case 5:
            ListarCircuito(&circuito);
            break;
        case 6:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            id_item = LerNumero("ID do item: ", 1, 100000);
            LerTexto("Nome do dono: ", dono, 29);
            peso = LerFloat("Peso (kg): ", 0.1f, 1000.0f);
            CadastrarItem(&bagagens, id_aviao, id_item, dono, peso);
            break;
        case 7:
        {
            ItemManifesto* r;
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            id_item = LerNumero("ID do item: ", 1, 100000);
            r = ConsultarItem(&bagagens, id_aviao, id_item);
            if (r) printf("Encontrado: dono %s, peso %.2fkg, status %d\n", r->dono, r->peso, r->status);
            else printf("Item nao encontrado.\n");
            break;
        }
        case 8:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            id_item = LerNumero("ID do item a remover: ", 1, 100000);
            if (RemoverAdministrativo(&bagagens, id_aviao, id_item))
                printf("Item removido do manifesto.\n");
            else
                printf("Item nao encontrado (ou nao pode ser removido).\n");
            break;
        case 9:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            EmbarcarItens(&bagagens, id_aviao);
            break;
        case 10:
        {
            int resultado;
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            resultado = DescarregarProximo(&bagagens, id_aviao);
            if (resultado == -1) printf("Porao vazio!\n");
            else if (resultado == -2) printf("Aviao ainda nao estacionou no portao - descarregamento ainda nao liberado.\n");
            else if (resultado == -3) printf("Nenhum registro de bagagem para esse aviao.\n");
            break;
        }
        case 11:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            id_item = LerNumero("ID da bagagem a isolar: ", 1, 100000);
            InspecaoEmergencia(&bagagens, id_aviao, id_item);
            break;
        case 12:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            ListarManifesto(&bagagens, id_aviao);
            break;
        case 13:
            id_aviao = LerNumero("ID da aeronave: ", 1, 1000);
            ListarPorao(&bagagens, id_aviao);
            break;
        default:
            if (opcao != 0)
                printf("Opcao invalida!\n");
        }
    } while (opcao != 0);

    DestruirFila(&fila);
    DestruirCircuito(&circuito);
    DestruirSistemaBagagem(&bagagens);
}

int main()
{
    system(CLEAR_COMMAND);
    SistemaAeroporto();
    return 0;
}
