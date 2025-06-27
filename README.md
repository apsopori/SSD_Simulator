# 🔧 SSD Simulation (FTL Strategies in C)

A C-based simulation of a Solid State Drive (SSD) that implements and compares three Flash Translation Layer (FTL) schemes: **Direct Mapping**, **Log-Structured Mapping**, and an **Idealized Mapping** strategy.  
Developed as a course project for IE411: Operating Systems.

---

## 📌 Features

- 🧠 **Three FTL Schemes:**
  - **Direct Mapping** — Simple but inefficient overwrite strategy.
  - **Log-Structured Mapping** — Write-efficient with dynamic mapping and garbage collection.
  - **Ideal Mapping** — Theoretical best-case mapping for comparison.

- 🔁 **Garbage Collection** with configurable watermarks.
- 🧮 **Statistics Tracking**: Logical and physical reads/writes/erases.
- 🔍 **State Dump**: Visual representation of SSD blocks, pages, and data.
- 📊 **Timing Analysis**: Compute total erase, write, and read time.

---

### 🔨 Compilation

```bash
gcc 202101001_SSD_Simulation.c -o ssd

### 💡 Flags

| Flag | Description |
|------|-------------|
| `-T` | FTL type: `direct`, `log`, `ideal` |
| `-l` | Number of logical pages |
| `-B` | Number of blocks |
| `-p` | Pages per block |
| `-G` | High watermark for garbage collection |
| `-g` | Low watermark for garbage collection |
| `-R` | Read time per page |
| `-W` | Write time per page |
| `-E` | Erase time per block |
| `-S` | Show SSD state after operations |
| `-s` | Show SSD performance stats |


### 🗂️ Code Structure

- `SSD_Struct`: Main data structure holding SSD metadata and mappings.
- `write_*()`: Functions for each FTL scheme.
- `garbage_collect()`: Cleans blocks and relocates live pages.
- `print_stats()`, `dump()`: Visual and performance output.
- `get_cursor()`, `is_block_free()`: Page allocation helpers.
