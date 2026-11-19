#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "meu_banco.h"

static double saldo_minimo(const Conta *c) {
    return c->tipo == POUPANCA ? 0.0 : -LIMITE_NEGATIVO;
}

static Resultado registrar(Conta *c, char op, double valor) {
    Historico *h = &c->historico;
    if (h->qtd == h->cap) {
        int nova_cap = h->cap == 0 ? 8 : h->cap * 2;
        Transacao *novo = realloc(h->itens, nova_cap * sizeof(Transacao));
        if (!novo) return ERRO_MEMORIA;
        h->itens = novo;
        h->cap = nova_cap;
    }
    h->itens[h->qtd].valor = valor;
    h->itens[h->qtd].op = op;
    h->qtd++;
    return OK;
}

static int senha_em_uso(const Banco *b, int senha) {
    for (int i = 0; i < b->qtd; i++)
        if (b->clientes[i].senha == senha) return 1;
    return 0;
}

static int fib(int n) {             /* fib(1)=1, fib(2)=1, fib(3)=2, ... */
    int a = 1, b = 1;
    for (int i = 3; i <= n; i++) { int t = a + b; a = b; b = t; }
    return b;
}


void banco_iniciar(Banco *b) {
    memset(b, 0, sizeof(Banco));
    b->proximo_numero = CODIGO_INICIAL;
}

void banco_liberar(Banco *b) {
    for (int i = 0; i < b->qtd; i++)
        for (int j = 0; j < b->clientes[i].qtd_contas; j++)
            free(b->clientes[i].contas[j].historico.itens);
    free(b->clientes);
    free(b->sorteios);
    banco_iniciar(b);
}

int tipos_compativeis(int a, int b) {
    if (a < CORRENTE || a > POUPANCA || b < CORRENTE || b > POUPANCA) return 0;
    if (a == b) return 0;
    if ((a == CORRENTE && b == SALARIO) || (a == SALARIO && b == CORRENTE)) return 0;
    return 1;
}

static void iniciar_conta(Conta *c, const char *nome, int numero, int tipo) {
    memset(c, 0, sizeof(Conta));
    strncpy(c->cliente, nome, TAM_NOME - 1);
    c->numero = numero;
    c->tipo = tipo;
}

Resultado banco_cadastrar_cliente(Banco *b, const char *nome, int tipo1, int tipo2, Cliente **cliente_out) {
    if (!nome || nome[0] == '\0') return ERRO_NOME_INVALIDO;
    if (tipo1 < CORRENTE || tipo1 > POUPANCA) return ERRO_TIPO_INVALIDO;
    if (tipo2 != 0) {
        if (tipo2 < CORRENTE || tipo2 > POUPANCA) return ERRO_TIPO_INVALIDO;
        if (!tipos_compativeis(tipo1, tipo2)) return ERRO_TIPOS_INCOMPATIVEIS;
    }
    if (b->proximo_numero > 9999) return ERRO_LIMITE_CLIENTES;


    int senha = 0, inicio = rand() % 9000;
    for (int k = 0; k < 9000; k++) {
        int s = 1000 + (inicio + k) % 9000;
        if (!senha_em_uso(b, s)) { senha = s; break; }
    }
    if (senha == 0) return ERRO_LIMITE_CLIENTES;

    if (b->qtd == b->cap) {
        int nova_cap = b->cap == 0 ? 4 : b->cap * 2;
        Cliente *novo = realloc(b->clientes, nova_cap * sizeof(Cliente));
        if (!novo) return ERRO_MEMORIA;
        b->clientes = novo;
        b->cap = nova_cap;
    }

    Cliente *c = &b->clientes[b->qtd];
    memset(c, 0, sizeof(Cliente));
    strncpy(c->nome, nome, TAM_NOME - 1);
    c->senha = senha;
    iniciar_conta(&c->contas[0], c->nome, b->proximo_numero, tipo1);
    c->qtd_contas = 1;
    if (tipo2 != 0) {
        iniciar_conta(&c->contas[1], c->nome, b->proximo_numero, tipo2);
        c->qtd_contas = 2;
    }
    b->proximo_numero++;
    b->qtd++;

    if (cliente_out) *cliente_out = c;
    return OK;
}

Conta *cliente_conta(Cliente *c, int tipo) {
    for (int i = 0; i < c->qtd_contas; i++)
        if (c->contas[i].tipo == tipo) return &c->contas[i];
    return NULL;
}

/* acesso = senha (4 digitos) + codigo do tipo da conta, ex.: 4821-01 */
Resultado banco_login(Banco *b, int senha, int tipo, Cliente **cliente_out, Conta **conta_out) {
    for (int i = 0; i < b->qtd; i++) {
        if (b->clientes[i].senha == senha) {
            Conta *conta = cliente_conta(&b->clientes[i], tipo);
            if (!conta) return ERRO_ACESSO_NEGADO;
            *cliente_out = &b->clientes[i];
            *conta_out = conta;
            return OK;
        }
    }
    return ERRO_ACESSO_NEGADO;
}


double cliente_saldo_total(const Cliente *c) {
    double total = 0.0;
    for (int i = 0; i < c->qtd_contas; i++) total += c->contas[i].saldo;
    return total;
}


double cliente_limite_emprestimo(const Cliente *c) {
    return PERCENTUAL_EMPRESTIMO * FATOR_EMPRESTIMO * cliente_saldo_total(c);
}

int cliente_negativado(const Cliente *c) {
    for (int i = 0; i < c->qtd_contas; i++)
        if (c->contas[i].saldo < 0) return 1;
    return 0;
}

Resultado cliente_verificar_emprestimo(Cliente *c) {
    if (!cliente_conta(c, CORRENTE)) return ERRO_SEM_CONTA_CORRENTE;
    if (c->emprestimo_realizado) return ERRO_EMPRESTIMO_JA_REALIZADO;
    Conta *p = cliente_conta(c, POUPANCA);
    if (!p || p->saldo <= 0) return ERRO_SEM_POUPANCA_COM_SALDO;
    if (cliente_limite_emprestimo(c) <= 0) return ERRO_ACIMA_DO_LIMITE;
    return OK;
}

int cliente_apto_emprestimo(Cliente *c) {
    return cliente_verificar_emprestimo(c) == OK;
}

int cliente_apto_titulo(Cliente *c) {
    Conta *s = cliente_conta(c, SALARIO);
    return s && !c->titulo && s->saldo >= TITULO_SALDO_MIN && s->saldo <= TITULO_SALDO_MAX;
}



Resultado conta_depositar(Conta *c, double valor) {
    if (valor <= 0) return ERRO_VALOR_INVALIDO;
    Resultado r = registrar(c, OP_DEPOSITO, valor);
    if (r != OK) return r;
    c->saldo += valor;
    return OK;
}

Resultado conta_sacar(Conta *c, double valor) {
    if (valor <= 0) return ERRO_VALOR_INVALIDO;
    if (c->saldo - valor < saldo_minimo(c)) return ERRO_SALDO_INSUFICIENTE;
    Resultado r = registrar(c, OP_SAQUE, valor);
    if (r != OK) return r;
    c->saldo -= valor;
    return OK;
}

Resultado cliente_solicitar_emprestimo(Cliente *c, double valor) {
    if (valor <= 0) return ERRO_VALOR_INVALIDO;
    Resultado r = cliente_verificar_emprestimo(c);
    if (r != OK) return r;
    if (valor > cliente_limite_emprestimo(c)) return ERRO_ACIMA_DO_LIMITE;

    Conta *corrente = cliente_conta(c, CORRENTE);
    r = registrar(corrente, OP_EMPRESTIMO, valor);
    if (r != OK) return r;
    corrente->saldo += valor;
    c->emprestimo_realizado = 1;
    c->valor_emprestimo = valor;
    return OK;
}

Resultado cliente_adquirir_titulo(Cliente *c) {
    if (c->titulo) return ERRO_TITULO_JA_ADQUIRIDO;
    Conta *s = cliente_conta(c, SALARIO);
    if (!s) return ERRO_SEM_CONTA_SALARIO;
    if (s->saldo < TITULO_SALDO_MIN || s->saldo > TITULO_SALDO_MAX) return ERRO_FORA_DA_FAIXA;
    c->titulo = 1;
    return OK;
}

void conta_extrato(const Conta *c, char filtro) {
    char num[16];
    numero_conta(c, num, sizeof num);
    printf("\n=== Extrato da conta %s ===\n", num);
    int mostrados = 0;
    for (int i = 0; i < c->historico.qtd; i++) {
        const Transacao *t = &c->historico.itens[i];
        if (filtro == '\0' || t->op == filtro) {
            printf("%.2f : %c\n", t->valor, t->op);
            mostrados++;
        }
    }
    if (mostrados == 0) printf("(sem movimentacoes)\n");
    printf("Saldo atual: R$ %.2f\n", c->saldo);
}

//Fibonacci

int peso_fibonacci(int posicao) {
    return fib(PARTICIPANTES_SORTEIO - posicao);
}

/* ordena por saldo total (decrescente); retorna quantos foram encontrados */
int banco_top_clientes(Banco *b, Cliente *top[PARTICIPANTES_SORTEIO]) {
    if (b->qtd == 0) return 0;
    char *usado = calloc(b->qtd, 1);
    if (!usado) return 0;

    int n = 0;
    while (n < PARTICIPANTES_SORTEIO && n < b->qtd) {
        int melhor = -1;
        for (int i = 0; i < b->qtd; i++) {
            if (usado[i]) continue;
            if (melhor < 0 || cliente_saldo_total(&b->clientes[i]) > cliente_saldo_total(&b->clientes[melhor]))
                melhor = i;
        }
        usado[melhor] = 1;
        top[n++] = &b->clientes[melhor];
    }
    free(usado);
    return n;
}

Resultado banco_sortear(Banco *b, Cliente **vencedor, double *premio) {
    Cliente *top[PARTICIPANTES_SORTEIO];
    if (banco_top_clientes(b, top) < PARTICIPANTES_SORTEIO) return ERRO_CLIENTES_INSUFICIENTES;
    for (int i = 0; i < PARTICIPANTES_SORTEIO; i++)
        if (!top[i]->titulo) return ERRO_SORTEIO_SEM_TITULO;

    int total = 0;
    for (int i = 0; i < PARTICIPANTES_SORTEIO; i++) total += peso_fibonacci(i);

    int sorteado = rand() % total, acumulado = 0, escolhido = 0;
    for (int i = 0; i < PARTICIPANTES_SORTEIO; i++) {
        acumulado += peso_fibonacci(i);
        if (sorteado < acumulado) { escolhido = i; break; }
    }

    Cliente *v = top[escolhido];
    Conta *salario = cliente_conta(v, SALARIO);
    if (!salario) return ERRO_SEM_CONTA_SALARIO;
    double valor = salario->saldo > 0 ? salario->saldo * FATOR_PREMIO : 0.0;

    if (b->qtd_sorteios == b->cap_sorteios) {
        int nova_cap = b->cap_sorteios == 0 ? 4 : b->cap_sorteios * 2;
        Contemplado *novo = realloc(b->sorteios, nova_cap * sizeof(Contemplado));
        if (!novo) return ERRO_MEMORIA;
        b->sorteios = novo;
        b->cap_sorteios = nova_cap;
    }
    if (valor > 0) {
        Resultado r = registrar(salario, OP_PREMIO, valor);
        if (r != OK) return r;
        salario->saldo += valor;
    }

    Contemplado *reg = &b->sorteios[b->qtd_sorteios++];
    strncpy(reg->cliente, v->nome, TAM_NOME - 1);
    reg->cliente[TAM_NOME - 1] = '\0';
    reg->numero = salario->numero;
    reg->premio = valor;

    *vencedor = v;
    *premio = valor;
    return OK;
}



void banco_historico(Banco *b) {
    char num[16];
    double total = 0.0;
    for (int i = 0; i < b->qtd; i++)
        for (int j = 0; j < b->clientes[i].qtd_contas; j++)
            total += b->clientes[i].contas[j].saldo;

    printf("\n== Saldo Total do banco == | R$ %.2f\n", total);

    printf("\n== Clientes Aptos a Emprestimos == ||\n");
    for (int i = 0; i < b->qtd; i++) {
        Cliente *c = &b->clientes[i];
        if (cliente_apto_emprestimo(c)) {
            numero_conta(cliente_conta(c, CORRENTE), num, sizeof num);
            printf("%s | %s | R$ %.2f\n", c->nome, num, cliente_limite_emprestimo(c));
        }
    }

    printf("\n== Clientes com Saldo Negativo na Conta Salario == ||\n");
    for (int i = 0; i < b->qtd; i++) {
        Cliente *c = &b->clientes[i];
        Conta *s = cliente_conta(c, SALARIO);
        if (s && s->saldo < 0) {
            numero_conta(s, num, sizeof num);
            printf("%s | %s | R$ %.2f\n", c->nome, num, s->saldo);
        }
    }

    printf("\n== Clientes Aptos a Compra de Titulos == ||\n");
    for (int i = 0; i < b->qtd; i++) {
        Cliente *c = &b->clientes[i];
        if (cliente_apto_titulo(c)) {
            Conta *s = cliente_conta(c, SALARIO);
            numero_conta(s, num, sizeof num);
            printf("%s | %s | R$ %.2f\n", c->nome, num, s->saldo);
        }
    }

    printf("\n== Clientes Contemplado no Sorteio == ||\n");
    for (int i = 0; i < b->qtd_sorteios; i++) {
        const Contemplado *k = &b->sorteios[i];
        printf("%s | %04d-%02d | R$ %.2f\n", k->cliente, k->numero, SALARIO, k->premio);
    }
}


void numero_conta(const Conta *c, char *buf, size_t tam) {
    snprintf(buf, tam, "%04d-%02d", c->numero, c->tipo);
}

const char *nome_tipo(int tipo) {
    switch (tipo) {
        case CORRENTE: return "Corrente";
        case SALARIO:  return "Salario";
        case POUPANCA: return "Poupanca";
        default:       return "Desconhecido";
    }
}

const char *mensagem_erro(Resultado r) {
    switch (r) {
        case OK:                           return "Operacao realizada com sucesso.";
        case ERRO_MEMORIA:                 return "Erro de memoria.";
        case ERRO_VALOR_INVALIDO:          return "Valor invalido.";
        case ERRO_NOME_INVALIDO:           return "Nome invalido.";
        case ERRO_TIPO_INVALIDO:           return "Tipo de conta invalido (use 1, 2 ou 3).";
        case ERRO_TIPOS_INCOMPATIVEIS:     return "Combinacao de contas nao permitida (salario e corrente nao podem coexistir; tipos devem ser diferentes).";
        case ERRO_LIMITE_CLIENTES:         return "Limite de clientes do banco atingido.";
        case ERRO_ACESSO_NEGADO:           return "Senha ou tipo de conta invalidos.";
        case ERRO_SALDO_INSUFICIENTE:      return "Saldo insuficiente.";
        case ERRO_SEM_CONTA_CORRENTE:      return "Voce precisa ter uma conta corrente.";
        case ERRO_SEM_CONTA_SALARIO:       return "Voce precisa ter uma conta salario.";
        case ERRO_SEM_POUPANCA_COM_SALDO:  return "Para o emprestimo, voce precisa ter poupanca com saldo.";
        case ERRO_EMPRESTIMO_JA_REALIZADO: return "Voce ja realizou um emprestimo (limite de 1).";
        case ERRO_ACIMA_DO_LIMITE:         return "Valor acima do limite de emprestimo disponivel.";
        case ERRO_FORA_DA_FAIXA:           return "O saldo da conta salario precisa estar entre 1000 e 1500.";
        case ERRO_TITULO_JA_ADQUIRIDO:     return "Voce ja possui titulo de capitalizacao.";
        case ERRO_CLIENTES_INSUFICIENTES:  return "O sorteio exige pelo menos 5 clientes cadastrados.";
        case ERRO_SORTEIO_SEM_TITULO:      return "Sorteio cancelado: todos os 5 participantes precisam ter titulo de capitalizacao.";
        default:                           return "Erro desconhecido.";
    }
}
