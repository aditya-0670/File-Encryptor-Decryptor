
# File Encryptor — Parallel Encryption & Decryption in C++

## 📖 Project Overview

**File Encryptor** is a C++ application that demonstrates two different parallel‐processing strategies for file encryption and decryption:

1. **Multiprocessing** (via `fork()`)  
2. **Multithreading** (via `pthread` + shared memory + semaphores)

By splitting large input directories into chunks and processing them in parallel, this tool maximizes CPU utilization and reduces wall‐clock time for cryptographic operations.

---

## 🌲 Repository Structure

```

encrypty/
├── src/
│   └── app/
│       ├── processes/
│       │   ├── ProcessManagement.hpp    ← Task scheduler & process‐spawn logic
│       │   ├── ProcessManagement.cpp
│       │   ├── Task.hpp                 ← Represents one file‐chunk task
│       │   └── Task.cpp
│       ├── threading/
│       │   ├── ThreadPool.hpp           ← Thread manager & worker pool
│       │   ├── ThreadPool.cpp
│       │   └── SyncPrimitives.hpp       ← Shared memory + semaphores
│       └── common/
│           ├── CryptoEngine.hpp         ← AES/CBC implementation
│           └── CryptoEngine.cpp
├── test/                              ← Sample input directories & expected outputs
│   └── test2.txt
├── main.cpp                           ← CLI: reads directory + ENCRYPT/DECRYPT
├── Makefile                           ← Build rules for both branches
└── README.md                          ← You are here

````

---

## 🌿 Branches

### 1. `add/childProcessing` (Multiprocessing)

- **Key Files:**  
  - `src/app/processes/ProcessManagement.*`  
  - `src/app/processes/Task.*`

- **Flow:**
  1. **TaskScheduler** (in `ProcessManagement`) scans the input directory and splits files into _N_ chunks.
  2. For each chunk, `fork()` spawns a **child process**.
  3. Each child calls `CryptoEngine::encryptChunk()` or `decryptChunk()`.
  4. Parent waits (`waitpid`) for all children, then recombines output into final files.

- **Pros:** OS‐level isolation, no data races  
- **Cons:** Higher memory overhead, IPC complexity

---

### 2. `add/multithreading` (Multithreading + Shared Memory)

- **Key Files:**  
  - `src/app/threading/ThreadPool.*`  
  - `src/app/threading/SyncPrimitives.*`

- **Flow:**
  1. **ThreadPool** creates a shared memory region (`mmap`) sized to hold _M_ chunks.
  2. Launches _T_ POSIX threads; each thread:
     - Reads its assigned chunk from shared memory  
     - Calls `CryptoEngine::encryptChunk()` / `decryptChunk()`  
     - Writes back to its segment
  3. Semaphores (in `SyncPrimitives`) guard access to shared buffers.
  4. Main thread writes all processed chunks to disk.

- **Pros:** Lower overhead, fast context switches  
- **Cons:** Manual synchronization, risk of deadlocks/races

---

## 🏗️ Architecture & Data Flow

```plaintext
+-------------+
|  main.cpp   |  ── parses args ──► [TaskScheduler]
+-------------+                      ├─ splits directory into chunks
       │                             └─ pushes Task objects
       ▼
+----------------------+       +----------------------+       +----------------------+
| Multiprocessing Mode |──────►|  Encrypt/Decrypt     |◄──────| Multithreading Mode  |
|  (fork & children)   |       |    CryptoEngine      |       |  (pthread pool)      |
+----------------------+       +----------------------+       +----------------------+
       │                                 ▲                             │
       └────────► [ResultCollector] ◄────┘◄────────[SyncPrimitives]◄───┘
                    writes final files
````

* **TaskScheduler**

  * Scans `directory/`
  * Creates `Task { filepath, offset, length }`

* **Process/Thread Workers**

  * Fetch a `Task`
  * Call `CryptoEngine::process(buffer, key, iv)`
  * Signal completion

* **ResultCollector**

  * Merges processed buffers into full output files

---

## ⚙️ Build & Run

```bash
# 1. Clone & switch branch
git clone https://github.com/<you>/encrypty.git
cd encrypty
git checkout add/childProcessing   # or add/multithreading

# 2. (Optional) Python helper for test dirs
python3 -m venv venv
source venv/bin/activate
python makeDirs.py

# 3. Build
make

# 4. Run
./encrypty
# prompts:
#   Enter the directory path:  test/
#   Enter the action (encrypt/decrypt): ENCRYPT
```

Outputs are written alongside originals as `*.enc` or `*.dec` extensions.

---

## 🚀 Performance Tips

* **Chunk Size:**

  * Too small → high overhead
  * Too large → poor CPU‐I/O overlap
* **Worker Count:**

  * ≈ number of CPU cores (multithreading)
  * ≤ number of cores (forking)
* **I/O Optimizations:**

  * Use `O_DIRECT` or asynchronous I/O
  * Tune read/write buffer sizes

---

## 🎯 Future Enhancements

* **Benchmark Harness** to compare modes
* **Key Management Module** (e.g., HSM integration)
* **Distributed Mode** via MPI or ZeroMQ
* **GPU Offload** for CryptoEngine

---

## 🤝 Contributing

1. Fork the repo
2. Create a feature branch (`git checkout -b feat/your-feature`)
3. Commit your changes
4. Open a PR against `main`

Please adhere to the existing code style and add tests in `test/`!
