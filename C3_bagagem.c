/* ======================================================================
   COMPONENTE 3 - Despacho de Porao, Cadastro Previo e Inspecao
   ----------------------------------------------------------------------
   Duas estruturas trabalham juntas:

   1) MANIFESTO (cadastro previo) - lista encadeada simples. Guarda todos
      os itens autorizados antes da decolagem, na ordem em que foram
      embarcados. Permite consulta arbitraria (por id) e remocao
      administrativa antes do embarque - operacoes tipicas de uma lista,
      nao de uma pilha.

   2) PORAO FISICO - PILHA (LIFO). O enunciado diz que o operador so tem
      acesso imediato ao ULTIMO item embarcado na origem: isso e
      exatamente o comportamento de uma pilha, onde so se acessa o topo.
      Cada bagagem descarregada sofre "double-check" contra o manifesto.

   Procedimento de interceptacao de emergencia (bagagem no meio da pilha):
   usa uma PILHA AUXILIAR. Retira-se item a item do porao (empilhando na
   auxiliar) ate encontrar o alvo; o alvo e isolado (nao volta); depois
   a pilha auxiliar e devolvida ao porao, o que a inverte novamente e
   restaura a ordem original das demais bagagens.
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

enum StatusItem
{
    Cadastrado = 0,
    Embarcado,
    Descarregado,
    Isolado,
    RemovidoAdmin
};

typedef struct ItemManifesto
{
    int id;
    char dono[30];
    float peso;
    enum StatusItem status;
    struct ItemManifesto* prox;
} ItemManifesto;

typedef struct NoPorao /* no da pilha fisica do porao */
{
    int id;
    char dono[30];
    float peso;
    struct NoPorao* prox; /* aponta para o item logo abaixo na pilha */
} NoPorao;

typedef struct SistemaBagagem
{
    ItemManifesto* manifesto; /* base de dados / cadastro previo (ordem de embarque) */
    ItemManifesto* fim_manifesto;
    NoPorao* topo;             /* topo da pilha fisica do porao */
} SistemaBagagem;

void CriarSistema(SistemaBagagem* sistema)
{
    sistema->manifesto = NULL;
    sistema->fim_manifesto = NULL;
    sistema->topo = NULL;
}

/* Cadastro previo (antes da decolagem na origem) - insere no fim, preservando ordem de embarque */
void CadastrarItem(SistemaBagagem* sistema, int id, char dono[30], float peso)
{
    ItemManifesto* novo = malloc(sizeof(ItemManifesto));
    novo->id = id;
    strcpy(novo->dono, dono);
    novo->peso = peso;
    novo->status = Cadastrado;
    novo->prox = NULL;

    if (sistema->manifesto == NULL)
        sistema->manifesto = novo;
    else
        sistema->fim_manifesto->prox = novo;

    sistema->fim_manifesto = novo;
}

ItemManifesto* ConsultarItem(SistemaBagagem* sistema, int id)
{
    ItemManifesto* atual = sistema->manifesto;
    while (atual)
    {
        if (atual->id == id)
            return atual;
        atual = atual->prox;
    }
    return NULL;
}

/* Remocao administrativa antes do embarque. Retorna 1 se removeu, 0 se nao encontrou */
int RemoverAdministrativo(SistemaBagagem* sistema, int id)
{
    ItemManifesto* atual = sistema->manifesto;
    ItemManifesto* anterior = NULL;

    while (atual)
    {
        if (atual->id == id)
        {
            if (atual->status != Cadastrado)
            {
                printf("Item ja embarcado/processado, nao pode ser removido administrativamente.\n");
                return 0;
            }
            if (anterior == NULL)
                sistema->manifesto = atual->prox;
            else
                anterior->prox = atual->prox;
            if (atual == sistema->fim_manifesto)
                sistema->fim_manifesto = anterior;
            free(atual);
            return 1;
        }
        anterior = atual;
        atual = atual->prox;
    }
    return 0;
}

/* Embarque fisico: empilha no porao todos os itens do manifesto ainda "Cadastrado",
   na ordem em que foram cadastrados (o ultimo cadastrado fica no topo da pilha) */
void EmbarcarItens(SistemaBagagem* sistema)
{
    ItemManifesto* atual = sistema->manifesto;
    NoPorao* novo;

    while (atual)
    {
        if (atual->status == Cadastrado)
        {
            novo = malloc(sizeof(NoPorao));
            novo->id = atual->id;
            strcpy(novo->dono, atual->dono);
            novo->peso = atual->peso;
            novo->prox = sistema->topo;
            sistema->topo = novo;

            atual->status = Embarcado;
        }
        atual = atual->prox;
    }
    printf("Itens cadastrados foram embarcados no porao.\n");
}

/* Descarrega o item do topo da pilha e faz o double-check contra o manifesto.
   Retorna: 1 = ok, 0 = alerta de inconformidade, -1 = porao vazio */
int DescarregarProximo(SistemaBagagem* sistema)
{
    NoPorao* topo = sistema->topo;
    ItemManifesto* referencia;

    if (topo == NULL)
        return -1;

    referencia = ConsultarItem(sistema, topo->id);

    if (referencia == NULL)
    {
        printf("*** ALERTA: item ID %d retirado do porao NAO CONSTA no cadastro previo! ***\n", topo->id);
        sistema->topo = topo->prox;
        free(topo);
        return 0;
    }

    if (strcmp(referencia->dono, topo->dono) != 0 || referencia->peso != topo->peso)
    {
        printf("*** ALERTA: item ID %d com divergencia de dados (dono/peso) em relacao ao cadastro! ***\n", topo->id);
        sistema->topo = topo->prox;
        free(topo);
        return 0;
    }

    printf("Item ID %d (%s, %.2fkg) descarregado e conferido com sucesso.\n",
           topo->id, topo->dono, topo->peso);
    referencia->status = Descarregado;
    sistema->topo = topo->prox;
    free(topo);
    return 1;
}

/*
   Procedimento de interceptacao de emergencia:
   A) Remove todas as bagagens do porao ate encontrar a bagagem alvo, guardando-as
      temporariamente em uma pilha auxiliar (empilhamento inverte a ordem uma vez).
   B) Isola a bagagem alvo (marca no manifesto, nao volta ao porao).
   C) Devolve a pilha auxiliar ao porao (empilhamento inverte a ordem de novo),
      restaurando a ORDEM ORIGINAL das demais bagagens, sem o item isolado.
*/
void InspecaoEmergencia(SistemaBagagem* sistema, int id_alvo)
{
    NoPorao* auxiliar = NULL;
    NoPorao* temp;
    ItemManifesto* referencia;
    int encontrado = 0;

    while (sistema->topo != NULL)
    {
        temp = sistema->topo;
        sistema->topo = temp->prox;

        if (temp->id == id_alvo)
        {
            referencia = ConsultarItem(sistema, temp->id);
            if (referencia)
                referencia->status = Isolado;

            printf("Bagagem ID %d ISOLADA para inspecao (dono: %s, peso: %.2fkg).\n",
                   temp->id, temp->dono, temp->peso);
            free(temp);
            encontrado = 1;
            break;
        }

        /* ainda nao chegou no alvo: guarda na pilha auxiliar */
        temp->prox = auxiliar;
        auxiliar = temp;
    }

    if (!encontrado)
        printf("Bagagem ID %d nao encontrada no porao (ja descarregada ou inexistente).\n", id_alvo);

    /* Restaura (na ordem original) tudo que foi retirado, com ou sem sucesso na busca */
    while (auxiliar != NULL)
    {
        temp = auxiliar;
        auxiliar = temp->prox;
        temp->prox = sistema->topo;
        sistema->topo = temp;
    }
}

void ListarPorao(SistemaBagagem* sistema)
{
    NoPorao* atual = sistema->topo;
    printf("\n\t\tPorao (topo -> fundo):\n");
    if (!atual) printf("(vazio)\n");
    while (atual)
    {
        printf("ID: %d | Dono: %s | Peso: %.2fkg\n", atual->id, atual->dono, atual->peso);
        atual = atual->prox;
    }
}

void ListarManifesto(SistemaBagagem* sistema)
{
    ItemManifesto* atual = sistema->manifesto;
    const char* nomes_status[] = {"Cadastrado", "Embarcado", "Descarregado", "Isolado", "Removido"};

    printf("\n\t\tManifesto:\n");
    if (!atual) printf("(vazio)\n");
    while (atual)
    {
        printf("ID: %d | Dono: %s | Peso: %.2fkg | Status: %s\n",
               atual->id, atual->dono, atual->peso, nomes_status[atual->status]);
        atual = atual->prox;
    }
}

void DestruirSistema(SistemaBagagem* sistema)
{
    ItemManifesto* am = sistema->manifesto;
    ItemManifesto* pm;
    NoPorao* ap = sistema->topo;
    NoPorao* pp;

    while (am) { pm = am->prox; free(am); am = pm; }
    while (ap) { pp = ap->prox; free(ap); ap = pp; }

    sistema->manifesto = NULL;
    sistema->fim_manifesto = NULL;
    sistema->topo = NULL;
}

void SistemaBagagens()
{
    system(CLEAR_COMMAND);

    SistemaBagagem sistema;
    int opcao, id;
    char dono[30];
    float peso;

    CriarSistema(&sistema);

    do
    {
        opcao = LerNumero(
            "\n\t0 - Sair\n\t"
            "1 - Cadastrar item no manifesto (pre-voo)\n\t"
            "2 - Consultar item no manifesto\n\t"
            "3 - Remocao administrativa (pre-embarque)\n\t"
            "4 - Embarcar itens cadastrados (empilhar no porao)\n\t"
            "5 - Descarregar proximo item do porao (com double-check)\n\t"
            "6 - Inspecao de emergencia (isolar item especifico)\n\t"
            "7 - Listar manifesto\n\t"
            "8 - Listar porao\n\t"
            "Selecione: ", 0, 8);

        switch (opcao)
        {
        case 1:
            system(CLEAR_COMMAND);

            id = LerNumero("ID do item: ", 1, 100000);
            LerTexto("Nome do dono: ", dono, 29);
            peso = LerFloat("Peso (kg): ", 0.1f, 1000.0f);
            CadastrarItem(&sistema, id, dono, peso);
            break;
        case 2:
        {
            system(CLEAR_COMMAND);
            ItemManifesto* r;
            id = LerNumero("ID do item: ", 1, 100000);
            r = ConsultarItem(&sistema, id);
            if (r)
                printf("Encontrado: dono %s, peso %.2fkg, status %d\n", r->dono, r->peso, r->status);
            else
                printf("Item nao encontrado no manifesto.\n");
            break;
        }
        case 3:
            system(CLEAR_COMMAND);
            id = LerNumero("ID do item a remover: ", 1, 100000);
            if (RemoverAdministrativo(&sistema, id))
                printf("Item removido do manifesto.\n");
            else
                printf("Item nao encontrado (ou nao pode ser removido).\n");
            break;
        case 4:
            system(CLEAR_COMMAND);
            EmbarcarItens(&sistema);
            break;
        case 5:
            system(CLEAR_COMMAND);
            if (DescarregarProximo(&sistema) == -1)
                printf("Porao vazio!\n");
            break;
        case 6:
            system(CLEAR_COMMAND);
            id = LerNumero("ID da bagagem a isolar: ", 1, 100000);
            InspecaoEmergencia(&sistema, id);
            break;
        case 7:
            system(CLEAR_COMMAND);
            ListarManifesto(&sistema);
            break;
        case 8:
            system(CLEAR_COMMAND);
            ListarPorao(&sistema);
            break;
        default:
            if (opcao != 0)
                printf("Opcao invalida!\n");
        }
    } while (opcao != 0);

    DestruirSistema(&sistema);
}

int main()
{
    system(CLEAR_COMMAND);
    SistemaBagagens();
    return 0;
}
