/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    COMPONENTE 1 - Aproximacao e Autorizacao de Pouso
   ----------------------------------------------------------------------
   Estrutura escolhida: lista encadeada simples, MANTIDA SEMPRE ORDENADA
   por prioridade (Emergencia=1, Comercial=2, Carga=3) no momento da
   insercao. Assim, autorizar o pouso sempre remove o INICIO da lista
   (operacao O(1)), o que satisfaz a regra "nivel inferior sempre pousa
   antes de nivel superior".

   Sugestoes da aula: Operador modificar a prioridade dos avioes ou colocar
   evento aleatorio para isso.

   Para isso será necessário remover o aviao da lista e criar o novo com
   as mesmas modificações ou sera que é possivel move-lo pela lista?
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
            printf("Valor invalido - Min %d, Max %d.\n\n",
                minimo, maximo);
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

        printf("Valor invalido - Max %d caracteres.\n\n",
            tamanho - 1);
    }
}

void SistemaPouso();

int main()
{
    system(CLEAR_COMMAND);

    SistemaPouso();

    return 0;
}

enum prioridade
{
    Emergencia = 1,
    Comercial,
    Carga
};

typedef struct Aviao
{
    char nome[20];
    enum prioridade prioridade;
    int id_aviao;
    struct Aviao* prox;
} Aviao;

typedef struct Avioes
{
    Aviao* inicio;
    Aviao* fim;
    int tam;
} Avioes;

void CriarAvioes(Avioes* avioes)
{
    avioes->inicio = NULL;
    avioes->fim = NULL;
    avioes->tam = 0;
}

/* Insere mantendo a lista ordenada por prioridade crescente (1 = mais urgente) */
void SolicitacaoPouso(Avioes* avioes, char nome[20], int prioridade, int id)
{
    Aviao* novo = malloc(sizeof(Aviao));
    Aviao* atual = NULL;

    if (!novo)
        return;

    strcpy(novo->nome, nome);
    novo->prioridade = prioridade;
    novo->id_aviao = id;
    novo->prox = NULL;

    /* Fila vazia -> novo é inicio e fim */
    if (avioes->inicio == NULL)
    {
        avioes->inicio = novo;
        avioes->fim = novo;
        avioes->tam++;
        return;
    }

    /* Prioridade maior que a do primeiro -> entra na frente de tudo */
    if (prioridade < avioes->inicio->prioridade)
    {
        novo->prox = avioes->inicio;
        avioes->inicio = novo;
        avioes->tam++;
        return;
    }

    /* Procura o ponto onde a prioridade do novo ja nao é mais menor */
    atual = avioes->inicio;
    while (atual->prox != NULL && atual->prox->prioridade <= prioridade)
    {
        atual = atual->prox;
    }

    novo->prox = atual->prox;
    atual->prox = novo;

    /* Se inseriu depois do ultimo no, atualiza o fim */
    if (novo->prox == NULL)
        avioes->fim = novo;

    avioes->tam++;
}

/* Remove sempre o inicio, que é o de maior prioridade */
Aviao* PermitirPousoPrioritario(Avioes* avioes)
{
    Aviao* remover = NULL;

    if (avioes->inicio)
    {
        remover = avioes->inicio;
        avioes->inicio = remover->prox;

        if (avioes->inicio == NULL)
            avioes->fim = NULL;

        avioes->tam--;
    }
    return remover;
}

void ListarAvioes(Avioes avioes)
{
    Aviao* aviao = avioes.inicio;

    printf("\n\t\tQuantidade de avioes: %d\n", avioes.tam);

    while (aviao)
    {
        printf("Nome: %s\n" "ID: %d\n" "Prioridade: %d\n\n",
               aviao->nome, aviao->id_aviao, aviao->prioridade);

        aviao = aviao->prox;
    }
}

void DestruirFila(Avioes* avioes)
{
    Aviao* atual = avioes->inicio;
    Aviao* proximo;

    while (atual)
    {
        proximo = atual->prox;
        free(atual);
        atual = proximo;
    }

    avioes->inicio = NULL;
    avioes->fim = NULL;
    avioes->tam = 0;
}

void SistemaPouso()
{
    system(CLEAR_COMMAND);

    int opcao = 0, id = 0, prioridade = 0;
    char nome[20];

    Aviao* remover = NULL;

    Avioes avioes;

    CriarAvioes(&avioes);

    do
    {
        opcao = LerNumero(
            "\n\t0 - Sair\n\t"
            "1 - Entrada de aviao\n\t"
            "2 - Liberar pouso prioritario\n\t"
            "3 - Listar solicitacoes de pouso\n\t"
            "Selecione: ", 0, 3);

        switch (opcao)
        {
        case 1:
            system(CLEAR_COMMAND);

            LerTexto("Digite o nome da aeronave: ", nome, 19);

            id = LerNumero("Digite o id da aeronave: ", 1, 100);

            prioridade = LerNumero("Digite a prioridade (1 a 3, sendo 1 mais urgente): ", 1, 3);

            SolicitacaoPouso(&avioes, nome, prioridade, id);

            system(CLEAR_COMMAND);
            break;
        case 2:
            system(CLEAR_COMMAND);

            remover = PermitirPousoPrioritario(&avioes);

            if (remover)
            {
                printf("Aviao - Nome: %s | ID: %d | Prioridade: %d | O pouso foi autorizado!\n",
                    remover->nome, remover->id_aviao, remover->prioridade);
                free(remover);
            }
            else
            {
                printf("Nenhum aviao no aeroporto!\n");
            }

            system(CLEAR_COMMAND);
            break;
        case 3:
            system(CLEAR_COMMAND);

            ListarAvioes(avioes);

            break;
        default:
            if (opcao != 0)
            {
                printf("Opcao Invalida!\n");
            }
        }
    } while (opcao != 0);

    DestruirFila(&avioes);
}
