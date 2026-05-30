/*
 * Corte de Hastes (Rod Cutting) - Programacao Dinamica
 * Disciplina: Projeto e Analise de Algoritmos - UFPI
 * Alunos: Matheus e Julio Cesar
 *
 * Compilar (Windows/MinGW):
 *   gcc -O0 -o corte_hastes corte_hastes.c -lpsapi
 * Executar:
 *   corte_hastes.exe
 */

#define NOMINMAX          /* impede windows.h de definir min/max como macro */
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>

/* ───────────────────────────────────────────────
   Utilitarios
─────────────────────────────────────────────── */

int maior(int a, int b) {           /* renomeado para evitar conflito com macro max */
    return (a > b) ? a : b;
}

/* Memoria de trabalho do processo em KB (Windows WorkingSet) */
long memoria_kb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return (long)(pmc.WorkingSetSize / 1024);
    return -1;
}

/* Tempo em segundos com alta resolucao (QueryPerformanceCounter) */
double tempo_atual() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}

/* ───────────────────────────────────────────────
   Contador global de chamadas recursivas
─────────────────────────────────────────────── */
long chamadas_recursivas = 0;   /* long + %ld: compativel com MinGW antigo */

/* ───────────────────────────────────────────────
   VERSAO RECURSIVA (forca bruta)
─────────────────────────────────────────────── */

/*
 * corte_recursivo(p, n):
 *   p[] = tabela de precos (p[i] = preco de uma haste de comprimento i)
 *   n   = comprimento da haste a ser cortada
 *   Retorna o lucro maximo possivel.
 *
 *   Recorrencia:
 *     r(0) = 0
 *     r(n) = max{ p[i] + r(n-i) }  para i = 1..n
 *
 *   Complexidade: O(2^n) — subproblemas recomputados multiplas vezes
 */
int corte_recursivo(int *p, int n) {
    chamadas_recursivas++;
    if (n == 0) return 0;

    int melhor = -1;
    int i, val;
    for (i = 1; i <= n; i++) {
        val = p[i] + corte_recursivo(p, n - i);
        if (val > melhor) melhor = val;
    }
    return melhor;
}

/* ───────────────────────────────────────────────
   VERSAO PD - TOP-DOWN (memoizacao)
─────────────────────────────────────────────── */

int corte_memo_aux(int *p, int n, int *memo) {
    int i, val, melhor;
    if (memo[n] >= 0) return memo[n];
    if (n == 0) { memo[0] = 0; return 0; }

    melhor = -1;
    for (i = 1; i <= n; i++) {
        val = p[i] + corte_memo_aux(p, n - i, memo);
        if (val > melhor) melhor = val;
    }
    memo[n] = melhor;
    return melhor;
}

/*
 * corte_memoizacao(p, n):
 *   Versao top-down: resolve recursivamente mas guarda cada
 *   subproblema em memo[]. Se memo[j] ja foi calculado,
 *   retorna diretamente sem recalcular.
 *
 *   Complexidade: O(n^2) tempo, O(n) memoria
 */
int corte_memoizacao(int *p, int n) {
    int i, res;
    int *memo = (int *)malloc((n + 1) * sizeof(int));
    for (i = 0; i <= n; i++) memo[i] = -1;
    res = corte_memo_aux(p, n, memo);
    free(memo);
    return res;
}

/* ───────────────────────────────────────────────
   VERSAO PD - BOTTOM-UP (tabulacao)
─────────────────────────────────────────────── */

/*
 * corte_pd(p, n, corte_ot):
 *   Versao bottom-up: preenche a tabela r[] de j=0 ate j=n.
 *
 *   r[0] = 0
 *   r[j] = max{ p[i] + r[j-i] }  para i = 1..j
 *
 *   Nao ha recursao — cada subproblema e resolvido uma unica vez.
 *
 *   Complexidade: O(n^2) tempo, O(n) memoria
 */
int corte_pd(int *p, int n, int *corte_ot) {
    int j, i, val, melhor, res;
    int *r = (int *)calloc(n + 1, sizeof(int));

    for (j = 1; j <= n; j++) {
        melhor = -1;
        for (i = 1; i <= j; i++) {
            val = p[i] + r[j - i];
            if (val > melhor) {
                melhor = val;
                if (corte_ot) corte_ot[j] = i;
            }
        }
        r[j] = melhor;
    }

    res = r[n];
    free(r);
    return res;
}

/* ───────────────────────────────────────────────
   Imprime a decomposicao otima de cortes
─────────────────────────────────────────────── */
void imprimir_decomposicao(int *p, int n) {
    int j, i, val, melhor, resto;
    int *r      = (int *)calloc(n + 1, sizeof(int));
    int *cortes = (int *)calloc(n + 1, sizeof(int));

    for (j = 1; j <= n; j++) {
        melhor = -1;
        for (i = 1; i <= j; i++) {
            val = p[i] + r[j - i];
            if (val > melhor) {
                melhor    = val;
                cortes[j] = i;
            }
        }
        r[j] = melhor;
    }

    printf("  Decomposicao otima: ");
    resto = n;
    while (resto > 0) {
        printf("%d", cortes[resto]);
        resto -= cortes[resto];
        if (resto > 0) printf(" + ");
    }
    printf("\n");
    free(r);
    free(cortes);
}

/* ───────────────────────────────────────────────
   Benchmark para um unico valor de n
─────────────────────────────────────────────── */
void benchmark(int *p, int n, int executar_recursiva) {
    double t_ini, t_fim;
    long   mem_antes, mem_depois;
    int    resultado;

    printf("+-------------------------------------------------+\n");
    printf("|  Haste de comprimento n = %-3d                   |\n", n);
    printf("+-------------------------------------------------+\n");

    /* Recursiva */
    if (executar_recursiva) {
        chamadas_recursivas = 0;
        mem_antes  = memoria_kb();
        t_ini      = tempo_atual();
        resultado  = corte_recursivo(p, n);
        t_fim      = tempo_atual();
        mem_depois = memoria_kb();

        printf("[RECURSIVA]\n");
        printf("  Lucro maximo    : %d\n",    resultado);
        printf("  Tempo           : %.2f ms\n", (t_fim - t_ini) * 1000);
        printf("  Memoria (KB)    : %ld KB  (delta: %+ld KB)\n",
               mem_depois, mem_depois - mem_antes);
        printf("  Chamadas recur. : %ld\n",   chamadas_recursivas);
    } else {
        printf("[RECURSIVA] Pulada — complexidade O(2^%d) e inviavel\n", n);
        printf("  Chamadas estimadas: 2^%d >> limite pratico\n", n);
    }

    /* PD Memoizacao */
    mem_antes  = memoria_kb();
    t_ini      = tempo_atual();
    resultado  = corte_memoizacao(p, n);
    t_fim      = tempo_atual();
    mem_depois = memoria_kb();

    printf("[PD - MEMOIZACAO (top-down)]\n");
    printf("  Lucro maximo    : %d\n",    resultado);
    printf("  Tempo           : %.2f ms\n", (t_fim - t_ini) * 1000);
    printf("  Memoria (KB)    : %ld KB  (delta: %+ld KB)\n",
           mem_depois, mem_depois - mem_antes);

    /* PD Bottom-Up */
    mem_antes  = memoria_kb();
    t_ini      = tempo_atual();
    resultado  = corte_pd(p, n, NULL);
    t_fim      = tempo_atual();
    mem_depois = memoria_kb();

    printf("[PD - TABULACAO (bottom-up)]\n");
    printf("  Lucro maximo    : %d\n",    resultado);
    printf("  Tempo           : %.2f ms\n", (t_fim - t_ini) * 1000);
    printf("  Memoria (KB)    : %ld KB  (delta: %+ld KB)\n",
           mem_depois, mem_depois - mem_antes);
    imprimir_decomposicao(p, n);

    printf("\n");
}

/* ───────────────────────────────────────────────
   MAIN
─────────────────────────────────────────────── */
int main(void) {
    /*
     * Tabela de precos classica (CLRS - Cormen et al.)
     *
     *  i  :  1   2   3   4   5   6   7   8   9  10
     *  p[i]: 1   5   8   9  10  17  17  20  24  30
     *
     * Para i > 10: p[i] = 3*i (extensao linear)
     */
    int p_base[] = {0, 1, 5, 8, 9, 10, 17, 17, 20, 24, 30};
    int MAX_BASE  = 10;
    int N_MAX     = 90;
    int i, k;

    int *p = (int *)calloc(N_MAX + 1, sizeof(int));
    for (i = 0; i <= N_MAX; i++)
        p[i] = (i <= MAX_BASE) ? p_base[i] : 3 * i;

    printf("==============================================\n");
    printf("   CORTE DE HASTES - Analise Comparativa\n");
    printf("   Recursiva  vs  Programacao Dinamica\n");
    printf("==============================================\n\n");

    printf("Tabela de precos (i=1..10, classica CLRS):\n");
    printf("  i    : ");
    for (i = 1; i <= MAX_BASE; i++) printf("%4d", i);
    printf("\n  p[i] : ");
    for (i = 1; i <= MAX_BASE; i++) printf("%4d", p[i]);
    printf("\n  (para i > 10: p[i] = 3*i)\n\n");
    /* Cenario 0: valores pequenos — recursiva viavel para comparacao */
    printf("=== CENARIO 0 --- n = 15  e  n = 20 ===\n\n");
    { int c0[] = {15, 20};
      for (k = 0; k < 2; k++) benchmark(p, c0[k], 1); }
   
    /* Cenario 1: n = 50 e n = 80 */
    printf("=== CENARIO 1 --- n = 50  e  n = 80 ===\n\n");
    { int c1[] = {50, 80};
      for (k = 0; k < 2; k++) benchmark(p, c1[k], 0); }

    /* Cenario 2: n = 60 e n = 90 */
    printf("=== CENARIO 2 --- n = 60  e  n = 90 ===\n\n");
    { int c2[] = {60, 90};
      for (k = 0; k < 2; k++) benchmark(p, c2[k], 0); }

    free(p);
    return 0;
}
