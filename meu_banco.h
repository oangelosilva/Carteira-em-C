#ifndef MEU_BANCO_H
#define MEU_BANCO_H

#include <stddef.h>

#define TAM_NOME        60
#define MAX_CONTAS      2        
#define CODIGO_INICIAL  1000     


#define CORRENTE 1
#define SALARIO  2
#define POUPANCA 3


#define OP_SAQUE      'S'
#define OP_DEPOSITO   'D'
#define OP_EMPRESTIMO 'E'
#define OP_PREMIO     'P'


#define FATOR_EMPRESTIMO      10.0
#define PERCENTUAL_EMPRESTIMO 0.25


#define LIMITE_NEGATIVO 500.0


#define TITULO_SALDO_MIN 1000.0
#define TITULO_SALDO_MAX 1500.0

/* Sorteio */
#define PARTICIPANTES_SORTEIO 5
#define FATOR_PREMIO          10.0

typedef enum {
    OK = 0,
    ERRO_MEMORIA,
    ERRO_VALOR_INVALIDO,
    ERRO_NOME_INVALIDO,
    ERRO_TIPO_INVALIDO,
    ERRO_TIPOS_INCOMPATIVEIS,
    ERRO_LIMITE_CLIENTES,
    ERRO_ACESSO_NEGADO,
    ERRO_SALDO_INSUFICIENTE,
    ERRO_SEM_CONTA_CORRENTE,
    ERRO_SEM_CONTA_SALARIO,
    ERRO_SEM_POUPANCA_COM_SALDO,
    ERRO_EMPRESTIMO_JA_REALIZADO,
    ERRO_ACIMA_DO_LIMITE,
    ERRO_FORA_DA_FAIXA,
    ERRO_TITULO_JA_ADQUIRIDO,
    ERRO_CLIENTES_INSUFICIENTES,
    ERRO_SORTEIO_SEM_TITULO
} Resultado;

typedef struct {
    double valor;             
    char op;                    
} Transacao;

typedef struct {
    Transacao *itens;
    int qtd;
    int cap;
} Historico;

typedef struct {
    char cliente[TAM_NOME];
    double saldo;
    int tipo;                   
    Historico historico;
    int numero;                 
} Conta;

typedef struct {
    char nome[TAM_NOME];
    int senha;                  
    Conta contas[MAX_CONTAS];
    int qtd_contas;
    int emprestimo_realizado;
    double valor_emprestimo;
    int titulo;                 
} Cliente;

typedef struct {
    char cliente[TAM_NOME];
    int numero;
    double premio;
} Contemplado;

typedef struct {
    Cliente *clientes;         
    int qtd;
    int cap;
    int proximo_numero;
    Contemplado *sorteios;
    int qtd_sorteios;
    int cap_sorteios;
} Banco;

/* Ciclo de vida */
void banco_iniciar(Banco *b);
void banco_liberar(Banco *b);

/* Cadastro e acesso */
int tipos_compativeis(int tipo_a, int tipo_b);
Resultado banco_cadastrar_cliente(Banco *b, const char *nome, int tipo1, int tipo2, Cliente **cliente_out);
Resultado banco_login(Banco *b, int senha, int tipo, Cliente **cliente_out, Conta **conta_out);
Conta *cliente_conta(Cliente *c, int tipo);

/* Consultas sobre o cliente */
double cliente_saldo_total(const Cliente *c);
double cliente_limite_emprestimo(const Cliente *c);
int cliente_negativado(const Cliente *c);
int cliente_apto_emprestimo(Cliente *c);
int cliente_apto_titulo(Cliente *c);

/* Operacoes do cliente */
Resultado conta_depositar(Conta *c, double valor);
Resultado conta_sacar(Conta *c, double valor);
Resultado cliente_verificar_emprestimo(Cliente *c);
Resultado cliente_solicitar_emprestimo(Cliente *c, double valor);
Resultado cliente_adquirir_titulo(Cliente *c);
void conta_extrato(const Conta *c, char filtro);   

/* Sorteio */
int peso_fibonacci(int posicao);                   
int banco_top_clientes(Banco *b, Cliente *top[PARTICIPANTES_SORTEIO]);
Resultado banco_sortear(Banco *b, Cliente **vencedor, double *premio);

/* Historico do banco */
void banco_historico(Banco *b);

/* Utilidades */
void numero_conta(const Conta *c, char *buf, size_t tam);
const char *nome_tipo(int tipo);
const char *mensagem_erro(Resultado r);

#endif
