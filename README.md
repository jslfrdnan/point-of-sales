# Proyek Arsitektur dan Organisasi Komputer
## Optimasi Penjumlahan Array (Cache-Aware Program)

### Deskripsi
Aplikasi ini mendemonstrasikan bagaimana memahami arsitektur komputer dapat meningkatkan kinerja program secara signifikan. Program menghitung total elemen array besar dengan berbagai teknik optimasi.

### Konsep Arsitektur yang Ditunjukkan

#### 1. Spatial Locality
- Akses array secara berurutan memanfaatkan cache line
- Satu cache line (64 bytes) berisi 16 integers
- Sequential access menghasilkan cache hit rate ~94%

#### 2. Temporal Locality
- Data yang baru diakses tetap di cache
- Cache blocking memastikan data tidak ter-evict sebelum digunakan

#### 3. Load-Use Hazard
- Pipeline stall ketika instruksi membutuhkan hasil load yang belum selesai
- Diatasi dengan multiple accumulators

#### 4. Instruction-Level Parallelism (ILP)
- CPU modern dapat mengeksekusi multiple instruksi per cycle
- Loop unrolling dan multiple accumulators mengeksploitasi ILP

### Implementasi yang Dibandingkan

1. **Sequential Traversal** - Baseline dengan spatial locality
2. **Random Traversal** - Menunjukkan dampak cache miss
3. **Loop Unrolling 2x/4x/8x** - Mengurangi loop overhead
4. **Software Pipelining** - Multiple accumulators untuk menghindari hazard
5. **Optimal** - Kombinasi semua teknik

### Cara Compile dan Menjalankan

#### Menggunakan GCC langsung (Windows):
```bash
gcc -O0 -o array_optimization array_optimization.c
./array_optimization
```

#### Menggunakan Makefile (jika tersedia make):
```bash
make          # Build tanpa optimasi
make run      # Jalankan program
make compare  # Build semua versi (-O0, -O2, -O3)
```

### Hasil yang Diharapkan

| Implementasi | Speedup (vs baseline) |
|-------------|----------------------|
| Sequential | 1.00x (baseline) |
| Random Access | 0.05-0.1x (10-20x lebih lambat) |
| Unroll 2x | 1.1-1.3x |
| Unroll 4x | 1.2-1.5x |
| Unroll 8x | 1.3-1.7x |
| Software Pipeline | 1.5-2.5x |
| Optimal | 2.0-3.0x |

*Hasil bervariasi tergantung CPU dan kondisi sistem*

### Metrik yang Diukur

- **Execution Time** - Waktu eksekusi dalam milliseconds
- **Speedup** - Perbandingan dengan baseline
- **Cache Hit Rate** - Estimasi berdasarkan access pattern
- **CPI (Cycles Per Instruction)** - Estimasi efisiensi instruksi

### Struktur Kode

```
array_optimization.c
├── Konfigurasi (array size, cache parameters)
├── Fungsi Utilitas
│   ├── get_time_ms() - High-resolution timer
│   ├── init_array() - Inisialisasi array
│   └── generate_random_indices() - Untuk random access
├── Implementasi Sum
│   ├── sum_sequential() - Baseline
│   ├── sum_random() - Random access
│   ├── sum_unroll_2x/4x/8x() - Loop unrolling
│   ├── sum_software_pipeline() - Multiple accumulators
│   └── sum_optimal() - Kombinasi optimal
└── Main
    ├── Eksperimen 1: Sequential vs Random
    ├── Eksperimen 2: Loop Unrolling
    ├── Eksperimen 3: Software Pipelining
    └── Ringkasan dan Kesimpulan
```

### Referensi
- Computer Architecture: A Quantitative Approach (Hennessy & Patterson)
- What Every Programmer Should Know About Memory (Ulrich Drepper)
