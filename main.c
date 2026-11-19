#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "meu_banco.h"


static int fim_entrada = 0;   

static int ler_texto(const char *prompt, char *dest, size_t tam) {
    printf("%s", prompt);
    if (!fgets(dest, (int)tam, stdin)) {
        dest[0] = '\0';
        fim_entrada = 1;
        return 0;
    }
    char *nl = strchr(dest, '\n');
    if (nl) *nl = '\0';
    else {                             
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    return 1;
}

static int ler_int(const char *prompt) {
    char buf[64];
    ler_texto(prompt, buf, sizeof buf);
    return (int)strtol(buf, NULL, 10);
}

static double ler_double(const char *prompt) {
    char buf[64];
    ler_texto(prompt, buf, sizeof buf);
    for (char *p = buf; *p; p++) if (*p == ',') *p = '.';
    return strtod(buf, NULL);
}

static int ler_sim_nao(const char *prompt) {
    char buf[16];
    ler_texto(prompt, buf, sizeof buf);
    return tolower((unsigned char)buf[0]) == 's';
}

static void mostrar(Resultado r) {
    printf("%s %s\n", r == OK ? "[OK]" : "[ERRO]", mensagem_erro(r));
}

static void alerta_negativado(const Cliente *c) {
    for (int i = 0; i < c->qtd_contas; i++) {
        const Conta *conta = &c->contas[i];
        if (conta->saldo < 0) {
            char num[16];
            numero_conta(conta, num, sizeof num);
            printf("\n*** ALERTA: %s, a conta %s esta NEGATIVADA (R$ %.2f). Regularize o quanto antes! ***\n",
                   c->nome, num, conta->saldo);
        }
    }
}


static int ler_tipo(const char *prompt, int outro) {
    while (!fim_entrada) {
        int tipo = ler_int(prompt);
        if (tipo < CORRENTE || tipo > POUPANCA)
            printf("[ERRO] %s\n", mensagem_erro(ERRO_TIPO_INVALIDO));
        else if (outro != 0 && !tipos_compativeis(outro, tipo))
            printf("[ERRO] %s\n", mensagem_erro(ERRO_TIPOS_INCOMPATIVEIS));
        else
            return tipo;
    }
    return 0;
}

static void acao_abrir_banco(Banco *b) {
    int qtd = ler_int("Quantidade de clientes a cadastrar: ");
    if (qtd <= 0) { printf("[ERRO] Quantidade invalida.\n"); return; }

    for (int i = 1; i <= qtd && !fim_entrada; i++) {
        printf("\n--- Cliente %d de %d ---\n", i, qtd);

        char nome[128];
        do {
            ler_texto("Nome do cliente: ", nome, sizeof nome);
        } while (nome[0] == '\0' && !fim_entrada);
        if (fim_entrada) return;

        int tipo1 = ler_tipo("Tipo da conta (1-corrente / 2-salario / 3-poupanca): ", 0);
        if (!tipo1) return;

        int tipo2 = 0;
        if (ler_sim_nao("Abrir uma segunda conta? (s/n): ")) {
            tipo2 = ler_tipo("Tipo da segunda conta (1-corrente / 2-salario / 3-poupanca): ", tipo1);
            if (!tipo2) return;
        }

        Cliente *c = NULL;
        Resultado r = banco_cadastrar_cliente(b, nome, tipo1, tipo2, &c);
        mostrar(r);
        if (r == OK) {
            char num[16];
            printf("Contas abertas:\n");
            for (int k = 0; k < c->qtd_contas; k++) {
                numero_conta(&c->contas[k], num, sizeof num);
                printf("  %s (%s)\n", num, nome_tipo(c->contas[k].tipo));
            }
            printf("Senha do cliente: %04d\n", c->senha);
            printf("Acesso: ");
            for (int k = 0; k < c->qtd_contas; k++)
                printf("%04d-%02d%s", c->senha, c->contas[k].tipo, k + 1 < c->qtd_contas ? "  ou  " : "\n");
        }
    }
}

//Sorteio

static void acao_sorteio(Banco *b) {
    Cliente *top[PARTICIPANTES_SORTEIO];
    int n = banco_top_clientes(b, top);
    if (n < PARTICIPANTES_SORTEIO) { mostrar(ERRO_CLIENTES_INSUFICIENTES); return; }

    printf("\n=== Cinco maiores saldos ===\n");
    printf("%-4s %-25s %12s %6s %8s\n", "Pos", "Cliente", "Saldo total", "Peso", "Titulo");
    for (int i = 0; i < n; i++)
        printf("%-4d %-25s %12.2f %6d %8s\n", i + 1, top[i]->nome, cliente_saldo_total(top[i]),
               peso_fibonacci(i), top[i]->titulo ? "sim" : "nao");

    Cliente *vencedor = NULL;
    double premio = 0.0;
    Resultado r = banco_sortear(b, &vencedor, &premio);
    if (r != OK) { mostrar(r); return; }

    printf("\n>>> Contemplado: %s | Premio: R$ %.2f (creditado na conta salario)\n", vencedor->nome, premio);
}

//Cliente Interface

static void menu_cliente(Banco *b) {
    char buf[64];
    int senha = 0, tipo = 0;
    ler_texto("Acesso - senha + tipo da conta (ex.: 1234-01): ", buf, sizeof buf);
    if (fim_entrada) return;
    if (sscanf(buf, "%d-%d", &senha, &tipo) != 2) {
        printf("[ERRO] Formato invalido. Use SENHA-TIPO, ex.: 1234-01.\n");
        return;
    }

    Cliente *cli = NULL;
    Conta *conta = NULL;
    Resultado r = banco_login(b, senha, tipo, &cli, &conta);
    if (r != OK) { mostrar(r); return; }

    printf("\nBem-vindo(a), %s!\n", cli->nome);
    alerta_negativado(cli);

    int opcao;
    do {
        char num[16];
        numero_conta(conta, num, sizeof num);
        printf("\n--- Conta %s (%s) | Saldo: R$ %.2f ---\n", num, nome_tipo(conta->tipo), conta->saldo);
        printf("1 - Depositar\n");
        printf("2 - Sacar\n");
        printf("3 - Solicitar emprestimo\n");
        printf("4 - Adquirir titulos de capitalizacao\n");
        printf("5 - Solicitar extrato\n");
        printf("0 - Voltar\n");
        opcao = ler_int("Opcao: ");

        switch (opcao) {
            case 1:
                mostrar(conta_depositar(conta, ler_double("Valor do deposito: ")));
                break;
            case 2:
                mostrar(conta_sacar(conta, ler_double("Valor do saque: ")));
                alerta_negativado(cli);
                break;
            case 3: {
                Resultado v = cliente_verificar_emprestimo(cli);
                if (v != OK) { mostrar(v); break; }
                printf("Limite disponivel: R$ %.2f\n", cliente_limite_emprestimo(cli));
                mostrar(cliente_solicitar_emprestimo(cli, ler_double("Valor do emprestimo: ")));
                break;
            }
            case 4:
                mostrar(cliente_adquirir_titulo(cli));
                break;
            case 5: {
                char f[16];
                ler_texto("Operacao (S-saque, D-deposito, E-emprestimo, P-premio, Enter-todas): ", f, sizeof f);
                char filtro = f[0] ? (char)toupper((unsigned char)f[0]) : '\0';
                if (filtro && !strchr("SDEP", filtro)) { printf("[ERRO] Operacao invalida.\n"); break; }
                conta_extrato(conta, filtro);
                break;
            }
            case 0: break;
            default: printf("Opcao invalida.\n");
        }
    } while (opcao != 0 && !fim_entrada);
}


int main(void) {
    srand((unsigned)time(NULL));

    Banco banco;
    banco_iniciar(&banco);

    char entrada[64];
    int sair = 0;
    do {
        printf("\n===== BANCO DIGITAL =====\n");
        printf("1 - Abrir o banco\n");
        printf("2 - Consultar historico\n");
        printf("3 - Efetuar sorteio\n");
        printf("4 - Acesso do cliente\n");
        printf("s - Sair\n");
        ler_texto("Opcao: ", entrada, sizeof entrada);

        /* o programa so finaliza quando o usuario digita 's' */
        if (entrada[0] == 's' || entrada[0] == 'S' || fim_entrada) {
            sair = 1;
            printf("\nEncerrando...\n");
            continue;
        }

        switch ((int)strtol(entrada, NULL, 10)) {
            case 1: acao_abrir_banco(&banco); break;
            case 2: banco_historico(&banco);  break;
            case 3: acao_sorteio(&banco);     break;
            case 4: menu_cliente(&banco);     break;
            default: printf("Opcao invalida.\n");
        }
    } while (!sair);

    banco_liberar(&banco);
    return 0;
}
