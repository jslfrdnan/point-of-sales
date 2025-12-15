/*
 * ============================================================================
 * PROYEK ARSITEKTUR DAN ORGANISASI KOMPUTER
 * Optimasi Penjumlahan Array (Cache-Aware Program)
 * ============================================================================
 * 
 * Konsep Arsitektur yang Ditunjukkan:
 * 1. Spatial Locality  - Akses memori berurutan memanfaatkan cache line
 * 2. Temporal Locality - Menggunakan kembali data yang baru diakses
 * 3. Load-Use Hazard   - Delay antara load dan penggunaan data
 * 4. Instruction-Level Parallelism (ILP) - Multiple operations per cycle
 * 
 * Eksperimen:
 * - Sequential vs Random Traversal
 * - Loop Unrolling (2x, 4x, 8x)
 * - Software Pipelining
 * - Analisis Cache Hit/Miss
 * - Perbandingan CPI & Execution Time
 * 
 * Compile: gcc -O0 -o array_opt array_optimization.c
 * Run:     ./array_opt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// ============================================================================
// KONFIGURASI
// ============================================================================

#define ARRAY_SIZE      (16 * 1024 * 1024)  // 16 juta elemen (64 MB untuk int)
#define CACHE_LINE_SIZE 64                   // Ukuran cache line dalam bytes
#define L1_CACHE_SIZE   (32 * 1024)         // 32 KB L1 Cache
#define L2_CACHE_SIZE   (256 * 1024)        // 256 KB L2 Cache
#define L3_CACHE_SIZE   (8 * 1024 * 1024)   // 8 MB L3 Cache
#define NUM_ITERATIONS  3                    // Jumlah iterasi untuk rata-rata

// Simulasi cache untuk analisis
typedef struct {
    unsigned long long hits;
    unsigned long long misses;
    unsigned long long accesses;
} CacheStats;

typedef struct {
    double execution_time_ms;
    long long sum;
    unsigned long long instructions_estimated;
    unsigned long long memory_accesses;
    CacheStats cache_stats;
} BenchmarkResult;

// ============================================================================
// FUNGSI UTILITAS
// ============================================================================

// High-resolution timer menggunakan Windows Performance Counter
double get_time_ms() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
}

// Inisialisasi array dengan nilai random
void init_array(int *arr, int size) {
    srand(42);  // Fixed seed untuk reproducibility
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 100;
    }
}

// Generate array indeks untuk akses random
void generate_random_indices(int *indices, int size) {
    // Inisialisasi dengan indeks sequential
    for (int i = 0; i < size; i++) {
        indices[i] = i;
    }
    // Fisher-Yates shuffle
    srand(12345);
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

// Simulasi sederhana cache behavior
void simulate_cache_access(CacheStats *stats, unsigned long long address, 
                           unsigned long long *cache_tags, int cache_lines) {
    unsigned long long tag = address / CACHE_LINE_SIZE;
    int index = tag % cache_lines;
    
    stats->accesses++;
    
    if (cache_tags[index] == tag) {
        stats->hits++;
    } else {
        stats->misses++;
        cache_tags[index] = tag;
    }
}

void print_separator() {
    printf("================================================================================\n");
}

void print_header(const char *title) {
    printf("\n");
    print_separator();
    printf("  %s\n", title);
    print_separator();
}

// ============================================================================
// VERSI 1: SEQUENTIAL TRAVERSAL (Baseline)
// ============================================================================
/*
 * KONSEP: SPATIAL LOCALITY
 * 
 * Akses array secara berurutan memanfaatkan spatial locality.
 * Ketika CPU mengakses arr[0], cache line yang berisi arr[0]-arr[15]
 * (untuk cache line 64 bytes dan int 4 bytes) di-load ke cache.
 * Akses berikutnya ke arr[1], arr[2], ... akan menjadi cache hit.
 * 
 * Cache Line: |arr[0]|arr[1]|arr[2]|...|arr[15]|
 *              ^--- satu akses memori mengambil 16 elemen
 */
BenchmarkResult sum_sequential(int *arr, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    
    double start = get_time_ms();
    
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    
    // Estimasi instruksi: load + add + increment + compare + branch
    result.instructions_estimated = size * 5;
    
    // Estimasi cache: setiap cache line = 16 integers (64/4)
    // Sequential access = 1 miss per 16 accesses
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 2: RANDOM TRAVERSAL (Menunjukkan Cache Miss)
// ============================================================================
/*
 * KONSEP: CACHE MISS DAN MEMORY LATENCY
 * 
 * Akses random menghancurkan spatial locality.
 * Setiap akses kemungkinan besar akan menjadi cache miss karena
 * data yang diperlukan tidak ada di cache line yang sudah di-load.
 * 
 * Memory Latency:
 * - L1 Cache Hit:  ~4 cycles
 * - L2 Cache Hit:  ~12 cycles
 * - L3 Cache Hit:  ~40 cycles
 * - RAM Access:    ~200+ cycles
 * 
 * Ini menunjukkan pentingnya memahami hierarki memori!
 */
BenchmarkResult sum_random(int *arr, int *indices, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    
    double start = get_time_ms();
    
    for (int i = 0; i < size; i++) {
        sum += arr[indices[i]];
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    result.instructions_estimated = size * 6;  // Extra untuk index lookup
    
    // Random access = hampir setiap akses adalah miss
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size * 0.9;  // ~90% miss rate
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 3: LOOP UNROLLING 2x
// ============================================================================
/*
 * KONSEP: MENGURANGI LOOP OVERHEAD & MENINGKATKAN ILP
 * 
 * Loop overhead per iterasi:
 * - Increment counter (i++)
 * - Compare (i < size)
 * - Conditional branch
 * 
 * Dengan unrolling 2x, kita mengurangi overhead 50%!
 * 
 * Sebelum:          Sesudah (2x unroll):
 * for(i=0;i<n;i++)  for(i=0;i<n;i+=2)
 *   sum += arr[i];    sum += arr[i] + arr[i+1];
 * 
 * Instruksi per 2 elemen:
 * Sebelum: 10 instruksi (2 x 5)
 * Sesudah: 7 instruksi (2 load, 2 add, 1 inc, 1 cmp, 1 branch)
 */
BenchmarkResult sum_unroll_2x(int *arr, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    int i;
    
    double start = get_time_ms();
    
    // Main loop - proses 2 elemen per iterasi
    for (i = 0; i < size - 1; i += 2) {
        sum += arr[i] + arr[i + 1];
    }
    
    // Handle elemen sisa
    for (; i < size; i++) {
        sum += arr[i];
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    result.instructions_estimated = (size / 2) * 7 + (size % 2) * 5;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 4: LOOP UNROLLING 4x
// ============================================================================
/*
 * KONSEP: LEBIH BANYAK ILP
 * 
 * Dengan 4 operasi independen, CPU dapat mengeksekusi secara paralel
 * menggunakan multiple execution units.
 * 
 * CPU Modern memiliki:
 * - 2-4 ALU units
 * - 2 Load/Store units
 * - Out-of-order execution
 * 
 * 4 independent loads dapat di-issue secara bersamaan!
 */
BenchmarkResult sum_unroll_4x(int *arr, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    int i;
    
    double start = get_time_ms();
    
    // Main loop - proses 4 elemen per iterasi
    for (i = 0; i < size - 3; i += 4) {
        sum += arr[i] + arr[i + 1] + arr[i + 2] + arr[i + 3];
    }
    
    // Handle elemen sisa
    for (; i < size; i++) {
        sum += arr[i];
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    result.instructions_estimated = (size / 4) * 9 + (size % 4) * 5;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 5: LOOP UNROLLING 8x
// ============================================================================
/*
 * KONSEP: MAKSIMAL ILP UNTUK OPERASI SEDERHANA
 * 
 * 8 elemen per iterasi mendekati batas praktis untuk loop unrolling
 * karena:
 * 1. Register pressure - perlu menyimpan banyak nilai sementara
 * 2. Code size - loop yang terlalu besar tidak fit di instruction cache
 * 3. Diminishing returns - overhead sudah sangat kecil
 */
BenchmarkResult sum_unroll_8x(int *arr, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    int i;
    
    double start = get_time_ms();
    
    // Main loop - proses 8 elemen per iterasi
    for (i = 0; i < size - 7; i += 8) {
        sum += arr[i] + arr[i + 1] + arr[i + 2] + arr[i + 3] +
               arr[i + 4] + arr[i + 5] + arr[i + 6] + arr[i + 7];
    }
    
    // Handle elemen sisa
    for (; i < size; i++) {
        sum += arr[i];
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    result.instructions_estimated = (size / 8) * 13 + (size % 8) * 5;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 6: SOFTWARE PIPELINING (Multiple Accumulators)
// ============================================================================
/*
 * KONSEP: MENGHINDARI LOAD-USE HAZARD & DATA DEPENDENCY
 * 
 * LOAD-USE HAZARD:
 * Ketika instruksi membutuhkan hasil dari load yang belum selesai:
 * 
 *   LOAD R1, [mem]     ; Cycle 1-4 (memory latency)
 *   ADD R2, R2, R1     ; Cycle 5 - STALL menunggu R1!
 * 
 * SOLUSI: Multiple Accumulators
 * Dengan 4 accumulator terpisah (sum0, sum1, sum2, sum3):
 * - Tidak ada dependency antara accumulator
 * - CPU dapat mengeksekusi operasi secara parallel
 * - Load untuk sum1 dapat dimulai saat sum0 masih di-pipeline
 * 
 * Pipeline tanpa stall:
 *   LOAD R1, [arr+0]   ; Untuk sum0
 *   LOAD R2, [arr+4]   ; Untuk sum1 - tidak perlu tunggu R1
 *   LOAD R3, [arr+8]   ; Untuk sum2
 *   LOAD R4, [arr+12]  ; Untuk sum3
 *   ADD sum0, sum0, R1 ; R1 sudah ready
 *   ADD sum1, sum1, R2 ; R2 sudah ready
 *   ...
 */
BenchmarkResult sum_software_pipeline(int *arr, int size) {
    BenchmarkResult result = {0};
    
    // Multiple accumulators untuk menghindari data dependency
    long long sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
    int i;
    
    double start = get_time_ms();
    
    // Main loop dengan 4 independent accumulator
    for (i = 0; i < size - 3; i += 4) {
        sum0 += arr[i];
        sum1 += arr[i + 1];
        sum2 += arr[i + 2];
        sum3 += arr[i + 3];
    }
    
    // Handle elemen sisa
    for (; i < size; i++) {
        sum0 += arr[i];
    }
    
    // Gabungkan hasil
    long long total = sum0 + sum1 + sum2 + sum3;
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = total;
    result.memory_accesses = size;
    result.instructions_estimated = (size / 4) * 8 + (size % 4) * 5 + 3;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 7: KOMBINASI OPTIMAL (Unroll + Multiple Accumulators)
// ============================================================================
/*
 * KONSEP: KOMBINASI SEMUA OPTIMASI
 * 
 * Menggabungkan:
 * 1. Loop Unrolling 8x - Minimal loop overhead
 * 2. Multiple Accumulators - Menghindari data dependency
 * 3. Sequential Access - Maksimal cache utilization
 * 
 * Ini adalah teknik yang digunakan library high-performance
 * seperti BLAS, Intel MKL, dll.
 */
BenchmarkResult sum_optimal(int *arr, int size) {
    BenchmarkResult result = {0};
    
    // 8 independent accumulators
    long long sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
    long long sum4 = 0, sum5 = 0, sum6 = 0, sum7 = 0;
    int i;
    
    double start = get_time_ms();
    
    // Main loop - 8 elemen dengan 8 accumulator
    for (i = 0; i < size - 7; i += 8) {
        sum0 += arr[i];
        sum1 += arr[i + 1];
        sum2 += arr[i + 2];
        sum3 += arr[i + 3];
        sum4 += arr[i + 4];
        sum5 += arr[i + 5];
        sum6 += arr[i + 6];
        sum7 += arr[i + 7];
    }
    
    // Handle elemen sisa
    for (; i < size; i++) {
        sum0 += arr[i];
    }
    
    // Gabungkan hasil (tree reduction untuk mengurangi dependency)
    long long total = (sum0 + sum1) + (sum2 + sum3) + (sum4 + sum5) + (sum6 + sum7);
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = total;
    result.memory_accesses = size;
    result.instructions_estimated = (size / 8) * 12 + (size % 8) * 5 + 7;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// VERSI 8: CACHE-BLOCKING (untuk demonstrasi temporal locality)
// ============================================================================
/*
 * KONSEP: TEMPORAL LOCALITY DENGAN CACHE BLOCKING
 * 
 * Untuk operasi yang mengakses data multiple kali (seperti matrix multiply),
 * cache blocking memastikan data tetap di cache saat digunakan.
 * 
 * Dalam konteks sum, kita demonstrasikan dengan multiple passes
 * pada block yang sama untuk menunjukkan perbedaan kinerja.
 */
BenchmarkResult sum_cache_blocking(int *arr, int size) {
    BenchmarkResult result = {0};
    long long sum = 0;
    
    // Block size yang fit di L1 cache
    int block_size = L1_CACHE_SIZE / sizeof(int);
    
    double start = get_time_ms();
    
    // Process per block
    for (int block = 0; block < size; block += block_size) {
        int end = (block + block_size < size) ? block + block_size : size;
        
        // Sum dalam block - data tetap di L1 cache
        for (int i = block; i < end; i++) {
            sum += arr[i];
        }
    }
    
    double end = get_time_ms();
    
    result.execution_time_ms = end - start;
    result.sum = sum;
    result.memory_accesses = size;
    result.instructions_estimated = size * 5 + (size / block_size) * 3;
    
    result.cache_stats.accesses = size;
    result.cache_stats.misses = size / 16;
    result.cache_stats.hits = size - result.cache_stats.misses;
    
    return result;
}

// ============================================================================
// FUNGSI ANALISIS DAN OUTPUT
// ============================================================================

void print_result(const char *name, BenchmarkResult *result, BenchmarkResult *baseline) {
    double speedup = baseline->execution_time_ms / result->execution_time_ms;
    double hit_rate = (double)result->cache_stats.hits / result->cache_stats.accesses * 100;
    
    // Estimasi CPI (Cycles Per Instruction)
    // Asumsi: 3 GHz CPU, waktu dalam ms
    double cycles = result->execution_time_ms * 3000000;  // 3GHz = 3M cycles/ms
    double cpi = cycles / result->instructions_estimated;
    
    printf("%-30s | %10.2f ms | %6.2fx | %6.2f%% | %6.2f\n",
           name,
           result->execution_time_ms,
           speedup,
           hit_rate,
           cpi);
}

void print_detailed_analysis(const char *name, BenchmarkResult *result) {
    printf("\n--- %s ---\n", name);
    printf("Execution Time:      %.2f ms\n", result->execution_time_ms);
    printf("Sum Result:          %lld\n", result->sum);
    printf("Memory Accesses:     %llu\n", result->memory_accesses);
    printf("Est. Instructions:   %llu\n", result->instructions_estimated);
    printf("Cache Hits:          %llu (%.2f%%)\n", 
           result->cache_stats.hits,
           (double)result->cache_stats.hits / result->cache_stats.accesses * 100);
    printf("Cache Misses:        %llu (%.2f%%)\n",
           result->cache_stats.misses,
           (double)result->cache_stats.misses / result->cache_stats.accesses * 100);
    
    // Bandwidth calculation
    double bandwidth = (result->memory_accesses * sizeof(int)) / 
                       (result->execution_time_ms / 1000.0) / (1024 * 1024 * 1024);
    printf("Memory Bandwidth:    %.2f GB/s\n", bandwidth);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    printf("\n");
    print_separator();
    printf("  PROYEK ARSITEKTUR DAN ORGANISASI KOMPUTER\n");
    printf("  Optimasi Penjumlahan Array (Cache-Aware Program)\n");
    print_separator();
    
    // Informasi sistem
    printf("\nKonfigurasi:\n");
    printf("- Array Size:     %d elemen (%.2f MB)\n", 
           ARRAY_SIZE, (double)ARRAY_SIZE * sizeof(int) / (1024 * 1024));
    printf("- Cache Line:     %d bytes\n", CACHE_LINE_SIZE);
    printf("- L1 Cache:       %d KB\n", L1_CACHE_SIZE / 1024);
    printf("- L2 Cache:       %d KB\n", L2_CACHE_SIZE / 1024);
    printf("- L3 Cache:       %d MB\n", L3_CACHE_SIZE / (1024 * 1024));
    printf("- Iterations:     %d\n", NUM_ITERATIONS);
    
    // Alokasi memori
    printf("\nMengalokasikan memori...\n");
    int *arr = (int*)malloc(ARRAY_SIZE * sizeof(int));
    int *random_indices = (int*)malloc(ARRAY_SIZE * sizeof(int));
    
    if (!arr || !random_indices) {
        printf("ERROR: Gagal mengalokasikan memori!\n");
        return 1;
    }
    
    // Inisialisasi
    printf("Menginisialisasi array...\n");
    init_array(arr, ARRAY_SIZE);
    generate_random_indices(random_indices, ARRAY_SIZE);
    
    // Warmup - untuk mengisi cache
    printf("Warming up cache...\n");
    volatile long long warmup = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        warmup += arr[i];
    }
    
    // ========================================================================
    // EKSPERIMEN 1: SEQUENTIAL VS RANDOM
    // ========================================================================
    print_header("EKSPERIMEN 1: SEQUENTIAL VS RANDOM ACCESS");
    
    printf("\n[TEORI]\n");
    printf("Sequential access memanfaatkan SPATIAL LOCALITY:\n");
    printf("- Satu cache line (64 bytes) berisi 16 integers\n");
    printf("- Setelah load pertama, 15 akses berikutnya adalah cache hit\n");
    printf("- Expected cache hit rate: ~93.75%%\n\n");
    
    printf("Random access menghancurkan spatial locality:\n");
    printf("- Setiap akses kemungkinan besar cache miss\n");
    printf("- Cache miss penalty: ~200 cycles vs ~4 cycles untuk hit\n");
    printf("- Expected slowdown: 10-50x lebih lambat\n\n");
    
    BenchmarkResult seq_result = {0}, rand_result = {0};
    
    // Run multiple times dan ambil rata-rata
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        BenchmarkResult r1 = sum_sequential(arr, ARRAY_SIZE);
        BenchmarkResult r2 = sum_random(arr, random_indices, ARRAY_SIZE);
        
        seq_result.execution_time_ms += r1.execution_time_ms;
        seq_result.sum = r1.sum;
        seq_result.cache_stats = r1.cache_stats;
        seq_result.instructions_estimated = r1.instructions_estimated;
        seq_result.memory_accesses = r1.memory_accesses;
        
        rand_result.execution_time_ms += r2.execution_time_ms;
        rand_result.sum = r2.sum;
        rand_result.cache_stats = r2.cache_stats;
        rand_result.instructions_estimated = r2.instructions_estimated;
        rand_result.memory_accesses = r2.memory_accesses;
    }
    seq_result.execution_time_ms /= NUM_ITERATIONS;
    rand_result.execution_time_ms /= NUM_ITERATIONS;
    
    printf("[HASIL]\n");
    printf("%-25s: %.2f ms (baseline)\n", "Sequential Access", seq_result.execution_time_ms);
    printf("%-25s: %.2f ms (%.2fx lebih lambat)\n", "Random Access", 
           rand_result.execution_time_ms,
           rand_result.execution_time_ms / seq_result.execution_time_ms);
    
    printf("\n[ANALISIS]\n");
    printf("Slowdown factor: %.2fx\n", rand_result.execution_time_ms / seq_result.execution_time_ms);
    printf("Ini menunjukkan betapa pentingnya cache-friendly access pattern!\n");
    
    // ========================================================================
    // EKSPERIMEN 2: LOOP UNROLLING
    // ========================================================================
    print_header("EKSPERIMEN 2: LOOP UNROLLING");
    
    printf("\n[TEORI]\n");
    printf("Loop overhead per iterasi:\n");
    printf("- Increment (i++):        1 cycle\n");
    printf("- Compare (i < size):     1 cycle\n");
    printf("- Branch:                 1-20 cycles (misprediction)\n\n");
    
    printf("Dengan unrolling, overhead dikurangi:\n");
    printf("- 2x unroll: overhead berkurang 50%%\n");
    printf("- 4x unroll: overhead berkurang 75%%\n");
    printf("- 8x unroll: overhead berkurang 87.5%%\n\n");
    
    BenchmarkResult unroll2_result = {0}, unroll4_result = {0}, unroll8_result = {0};
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        BenchmarkResult r2 = sum_unroll_2x(arr, ARRAY_SIZE);
        BenchmarkResult r4 = sum_unroll_4x(arr, ARRAY_SIZE);
        BenchmarkResult r8 = sum_unroll_8x(arr, ARRAY_SIZE);
        
        unroll2_result.execution_time_ms += r2.execution_time_ms;
        unroll4_result.execution_time_ms += r4.execution_time_ms;
        unroll8_result.execution_time_ms += r8.execution_time_ms;
        
        unroll2_result.sum = r2.sum;
        unroll4_result.sum = r4.sum;
        unroll8_result.sum = r8.sum;
        
        unroll2_result.instructions_estimated = r2.instructions_estimated;
        unroll4_result.instructions_estimated = r4.instructions_estimated;
        unroll8_result.instructions_estimated = r8.instructions_estimated;
    }
    unroll2_result.execution_time_ms /= NUM_ITERATIONS;
    unroll4_result.execution_time_ms /= NUM_ITERATIONS;
    unroll8_result.execution_time_ms /= NUM_ITERATIONS;
    
    printf("[HASIL]\n");
    printf("%-20s | %12s | %8s | %12s\n", "Versi", "Waktu (ms)", "Speedup", "Est. Instr");
    printf("%-20s-+-%12s-+-%8s-+-%12s\n", "--------------------", "------------", "--------", "------------");
    printf("%-20s | %12.2f | %8s | %12llu\n", "Sequential (base)", 
           seq_result.execution_time_ms, "1.00x", seq_result.instructions_estimated);
    printf("%-20s | %12.2f | %7.2fx | %12llu\n", "Unroll 2x", 
           unroll2_result.execution_time_ms, 
           seq_result.execution_time_ms / unroll2_result.execution_time_ms,
           unroll2_result.instructions_estimated);
    printf("%-20s | %12.2f | %7.2fx | %12llu\n", "Unroll 4x", 
           unroll4_result.execution_time_ms,
           seq_result.execution_time_ms / unroll4_result.execution_time_ms,
           unroll4_result.instructions_estimated);
    printf("%-20s | %12.2f | %7.2fx | %12llu\n", "Unroll 8x", 
           unroll8_result.execution_time_ms,
           seq_result.execution_time_ms / unroll8_result.execution_time_ms,
           unroll8_result.instructions_estimated);
    
    // ========================================================================
    // EKSPERIMEN 3: SOFTWARE PIPELINING
    // ========================================================================
    print_header("EKSPERIMEN 3: SOFTWARE PIPELINING (MULTIPLE ACCUMULATORS)");
    
    printf("\n[TEORI]\n");
    printf("LOAD-USE HAZARD dalam pipeline:\n");
    printf("\n");
    printf("Single Accumulator (data dependency):\n");
    printf("  Cycle 1: LOAD R1, [arr+0]\n");
    printf("  Cycle 2: (stall - waiting for R1)\n");
    printf("  Cycle 3: (stall - waiting for R1)\n");
    printf("  Cycle 4: ADD sum, sum, R1    <- Baru bisa execute!\n");
    printf("  Cycle 5: LOAD R1, [arr+4]\n");
    printf("  ... (repeat stalls)\n\n");
    
    printf("Multiple Accumulators (no dependency):\n");
    printf("  Cycle 1: LOAD R1, [arr+0]    | LOAD R2, [arr+4]\n");
    printf("  Cycle 2: LOAD R3, [arr+8]    | LOAD R4, [arr+12]\n");
    printf("  Cycle 3: ADD sum0, sum0, R1  | ADD sum1, sum1, R2\n");
    printf("  Cycle 4: ADD sum2, sum2, R3  | ADD sum3, sum3, R4\n");
    printf("  ... (no stalls!)\n\n");
    
    BenchmarkResult pipeline_result = {0}, optimal_result = {0};
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        BenchmarkResult rp = sum_software_pipeline(arr, ARRAY_SIZE);
        BenchmarkResult ro = sum_optimal(arr, ARRAY_SIZE);
        
        pipeline_result.execution_time_ms += rp.execution_time_ms;
        optimal_result.execution_time_ms += ro.execution_time_ms;
        
        pipeline_result.sum = rp.sum;
        optimal_result.sum = ro.sum;
        
        pipeline_result.instructions_estimated = rp.instructions_estimated;
        optimal_result.instructions_estimated = ro.instructions_estimated;
    }
    pipeline_result.execution_time_ms /= NUM_ITERATIONS;
    optimal_result.execution_time_ms /= NUM_ITERATIONS;
    
    printf("[HASIL]\n");
    printf("%-25s | %12s | %8s\n", "Versi", "Waktu (ms)", "Speedup");
    printf("%-25s-+-%12s-+-%8s\n", "-------------------------", "------------", "--------");
    printf("%-25s | %12.2f | %8s\n", "Sequential (base)", 
           seq_result.execution_time_ms, "1.00x");
    printf("%-25s | %12.2f | %7.2fx\n", "4 Accumulators", 
           pipeline_result.execution_time_ms,
           seq_result.execution_time_ms / pipeline_result.execution_time_ms);
    printf("%-25s | %12.2f | %7.2fx\n", "8 Accumulators + Unroll", 
           optimal_result.execution_time_ms,
           seq_result.execution_time_ms / optimal_result.execution_time_ms);
    
    // ========================================================================
    // RINGKASAN HASIL
    // ========================================================================
    print_header("RINGKASAN HASIL DAN ANALISIS");
    
    printf("\n%-30s | %10s | %8s | %8s | %6s\n", 
           "Implementasi", "Waktu(ms)", "Speedup", "Hit Rate", "CPI*");
    printf("%-30s-+-%10s-+-%8s-+-%8s-+-%6s\n",
           "------------------------------", "----------", "--------", "--------", "------");
    
    // Calculate CPI untuk setiap versi
    double base_cycles = seq_result.execution_time_ms * 3000000;
    
    printf("%-30s | %10.2f | %8s | %7.2f%% | %6.2f\n",
           "1. Sequential (baseline)", seq_result.execution_time_ms, "1.00x",
           93.75, base_cycles / seq_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "2. Random Access", rand_result.execution_time_ms,
           rand_result.execution_time_ms / seq_result.execution_time_ms,
           10.0, (rand_result.execution_time_ms * 3000000) / rand_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "3. Loop Unroll 2x", unroll2_result.execution_time_ms,
           seq_result.execution_time_ms / unroll2_result.execution_time_ms,
           93.75, (unroll2_result.execution_time_ms * 3000000) / unroll2_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "4. Loop Unroll 4x", unroll4_result.execution_time_ms,
           seq_result.execution_time_ms / unroll4_result.execution_time_ms,
           93.75, (unroll4_result.execution_time_ms * 3000000) / unroll4_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "5. Loop Unroll 8x", unroll8_result.execution_time_ms,
           seq_result.execution_time_ms / unroll8_result.execution_time_ms,
           93.75, (unroll8_result.execution_time_ms * 3000000) / unroll8_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "6. Software Pipeline (4 acc)", pipeline_result.execution_time_ms,
           seq_result.execution_time_ms / pipeline_result.execution_time_ms,
           93.75, (pipeline_result.execution_time_ms * 3000000) / pipeline_result.instructions_estimated);
    
    printf("%-30s | %10.2f | %7.2fx | %7.2f%% | %6.2f\n",
           "7. Optimal (8 acc + unroll)", optimal_result.execution_time_ms,
           seq_result.execution_time_ms / optimal_result.execution_time_ms,
           93.75, (optimal_result.execution_time_ms * 3000000) / optimal_result.instructions_estimated);
    
    printf("\n* CPI = Cycles Per Instruction (estimasi berdasarkan 3 GHz CPU)\n");
    
    // Verifikasi hasil
    printf("\n[VERIFIKASI HASIL]\n");
    printf("Semua implementasi harus menghasilkan sum yang sama:\n");
    printf("- Sequential:      %lld\n", seq_result.sum);
    printf("- Random:          %lld\n", rand_result.sum);
    printf("- Unroll 2x:       %lld\n", unroll2_result.sum);
    printf("- Unroll 4x:       %lld\n", unroll4_result.sum);
    printf("- Unroll 8x:       %lld\n", unroll8_result.sum);
    printf("- Pipeline:        %lld\n", pipeline_result.sum);
    printf("- Optimal:         %lld\n", optimal_result.sum);
    
    int all_match = (seq_result.sum == rand_result.sum) &&
                    (seq_result.sum == unroll2_result.sum) &&
                    (seq_result.sum == unroll4_result.sum) &&
                    (seq_result.sum == unroll8_result.sum) &&
                    (seq_result.sum == pipeline_result.sum) &&
                    (seq_result.sum == optimal_result.sum);
    
    printf("Status: %s\n", all_match ? "PASSED - Semua hasil cocok!" : "FAILED - Ada perbedaan!");
    
    // ========================================================================
    // KESIMPULAN
    // ========================================================================
    print_header("KESIMPULAN");
    
    printf("\n1. SPATIAL LOCALITY (Sequential vs Random)\n");
    printf("   - Sequential access %.2fx lebih cepat dari random\n",
           rand_result.execution_time_ms / seq_result.execution_time_ms);
    printf("   - Cache hit rate: ~94%% (sequential) vs ~10%% (random)\n");
    printf("   - Lesson: Selalu akses memori secara berurutan jika memungkinkan\n\n");
    
    printf("2. LOOP UNROLLING\n");
    printf("   - Mengurangi loop overhead (branch, increment, compare)\n");
    printf("   - Best speedup: %.2fx (8x unroll)\n",
           seq_result.execution_time_ms / unroll8_result.execution_time_ms);
    printf("   - Lesson: Unrolling efektif untuk loop sederhana\n\n");
    
    printf("3. SOFTWARE PIPELINING\n");
    printf("   - Multiple accumulators menghindari load-use hazard\n");
    printf("   - Memungkinkan CPU mengeksploitasi ILP (Instruction-Level Parallelism)\n");
    printf("   - Best speedup: %.2fx (optimal version)\n",
           seq_result.execution_time_ms / optimal_result.execution_time_ms);
    printf("   - Lesson: Hindari data dependency dalam critical loop\n\n");
    
    printf("4. IMPLIKASI ARSITEKTUR\n");
    printf("   - Memahami hierarki memori sangat penting untuk kinerja\n");
    printf("   - CPU pipeline dapat stall karena data dependency\n");
    printf("   - Optimasi pada level source code dapat memberikan speedup signifikan\n");
    printf("   - Compiler optimization (-O2, -O3) menerapkan teknik serupa\n");
    
    print_separator();
    printf("\n");
    
    // Cleanup
    free(arr);
    free(random_indices);
    
    return 0;
}
