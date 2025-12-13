# Viva Colombia - Development Roadmap

## Vision: The Language That Has It All

```
┌─────────────────────────────────────────────────────────────────┐
│  VIVA: Simple as Go, Safe as Rust, Faster than Both            │
│                                                                 │
│  ✓ No garbage collector    ✓ No complex annotations            │
│  ✓ Compile-time safety     ✓ Fast compilation                  │
│  ✓ Zero-cost abstractions  ✓ First-class Spanish syntax        │
└─────────────────────────────────────────────────────────────────┘
```

**Build everything:** Kernels, games, banks, blockchains, web, mobile, OS

---

## Current Status: SELF-HOSTING ACHIEVED ✅

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  C Compiler     │────▶│  Bootstrap v3   │────▶│  Bootstrap v4   │
│  (archived)     │     │  (backup)       │     │  (main)         │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                              │                        │
                              └────── IDENTICAL ───────┘
                              MD5: 84cdcc71c77efddbf2d41b36d1f4f55c
```

**The Viva compiler compiles itself. C compiler no longer needed.**

### Working Features
| Feature | Syntax | Status |
|---------|--------|--------|
| Functions | `cancion name(): tipo { }` | ✅ |
| Variables | `decreto x: entero = valor;` | ✅ |
| Arithmetic | `+ - * /` | ✅ |
| Comparisons | `== != < > <= >=` | ✅ |
| Conditionals | `si (cond) { } sino { }` | ✅ |
| While loops | `mientras (cond) { }` | ✅ |
| Nested loops | ✅ | ✅ |
| Arrays | `decreto arr: [N]tipo;` | ✅ |
| Array expressions | `arr[i + j]` | ✅ |
| Function calls | `func(arg1, arg2)` | ✅ |
| Global variables | ✅ | ✅ |
| System calls | `escribir_sys(fd, buf, len)` | ✅ |
| Self-hosting | Compiles itself | ✅ |

### Current Limitations
- ❌ Recursion (argument passing bug)
- ❌ Structs
- ❌ Pointers
- ❌ Heap allocation
- ❌ Strings (only literals)
- ❌ Multi-platform (Linux x64 only)

---

## Phase 1: Foundation Completion

### 1.1 Fix Recursion [CRITICAL]
**Unlocks:** Recursive algorithms, tree traversal, parsers

```viva
cancion factorial(n: entero): entero {
    si (n < 2) { retorno 1; }
    retorno n * factorial(n - 1);  // Currently broken
}
```

**Problem:** Function arguments not passed correctly in recursive calls
**Solution:** Fix calling convention, proper stack frame setup

### 1.2 Add Structs
**Unlocks:** Complex data types, data modeling, protocols

```viva
estructura Punto {
    x: entero;
    y: entero;
}

decreto p: Punto;
p.x = 10;
p.y = 20;
```

### 1.3 Add Pointers
**Unlocks:** References, linked structures, dynamic data

```viva
decreto x: entero = 42;
decreto ptr: *entero = &x;
decreto valor: entero = *ptr;
```

---

## Phase 2: Memory System (THE INNOVATION)

### The Viva Memory Model: "Contextual Ownership"

**What makes Viva unique:** Memory strategy determined by CONTEXT, not annotations.

```
┌─────────────────────────────────────────────────────────────────┐
│                    VIVA MEMORY MODEL                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐       │
│   │  Stack  │   │  Arena  │   │  Owned  │   │ Counted │       │
│   │ (auto)  │   │ (bulk)  │   │ (move)  │   │ (share) │       │
│   └────┬────┘   └────┬────┘   └────┬────┘   └────┬────┘       │
│        │             │             │             │             │
│        └─────────────┴─────────────┴─────────────┘             │
│                          │                                      │
│              ┌───────────▼───────────┐                         │
│              │  CONTEXT DETERMINES   │                         │
│              │  WHICH STRATEGY USED  │                         │
│              └───────────────────────┘                         │
│                                                                 │
│   NO GARBAGE COLLECTOR. EVER. DETERMINISTIC. PREDICTABLE.     │
└─────────────────────────────────────────────────────────────────┘
```

### 2.1 Arena Allocator
**Unlocks:** Web servers, game engines, compilers

```viva
arena peticion {
    // All allocations use this arena
    usuario = cargar_usuario(id);
    datos = procesar(usuario);
    respuesta = generar(datos);
}  // Everything freed instantly - O(1)
```

**Why it's revolutionary:**
- No individual tracking
- No reference counting overhead
- Bulk free in O(1)
- Perfect for request/frame/phase patterns

### 2.2 Ownership (Simplified)
**No lifetime annotations. Compiler infers or asks.**

```viva
decreto datos = crear_datos();  // datos owns memory
usar(datos);                     // borrowed (inferred)
consumir(datos);                 // moved (inferred)
// datos gone, memory freed

// Only annotate when TRULY ambiguous (rare)
```

### 2.3 Semantic Lifetimes (Innovation)
**Instead of `'a, 'b, 'c` - meaningful names:**

```viva
// Memory tied to MEANINGFUL scopes
cancion manejar(): respuesta@peticion {
    config = cargar()@aplicacion;   // Lives for app lifetime
    usuario = obtener()@sesion;     // Lives for session
    datos = procesar()@peticion;    // Lives for this request
    retorno crear_respuesta(datos);
}
```

More readable than Rust. Self-documenting.

### 2.4 Compile-Time Proofs
**Zero runtime cost, total safety:**

```viva
// Compiler PROVES this is safe
decreto arr: [100]entero;
decreto i: indice{0..99};    // Type includes constraint
arr[i];                       // No bounds check needed!

// Compiler REJECTS this
decreto j: entero = 150;
arr[j];  // Error: cannot prove j < 100
```

---

## Phase 3: Real Applications

### 3.1 Strings
**Unlocks:** Text processing, web, JSON, protocols

```viva
decreto nombre: texto = "Viva Colombia";
decreto largo: entero = nombre.len;
escribir(nombre);
```

### 3.2 Dynamic Arrays
**Unlocks:** Lists, buffers, real applications

```viva
decreto lista: Vec<entero> = Vec.crear();
lista.agregar(42);
lista.agregar(100);
```

### 3.3 File I/O
**Unlocks:** Real applications, databases

```viva
decreto archivo = abrir("datos.txt");
decreto contenido = leer(archivo);
cerrar(archivo);
```

### 3.4 Networking
**Unlocks:** Web servers, distributed systems, blockchain

```viva
decreto servidor = escuchar(8080);
mientras (verdad) {
    decreto conexion = aceptar(servidor);
    arena peticion {
        manejar(conexion);
    }
}
```

---

## Phase 4: Advanced Features

### 4.1 Effect System
**Know at compile time what a function does:**

```viva
cancion pura calcular(x: entero): entero { }      // No side effects
cancion io guardar(datos: texto) { }               // Does I/O
cancion muta cambiar(arr: *[10]entero) { }        // Mutates

// Compiler enforces: pura functions cannot call io functions!
```

### 4.2 Refinement Types
**Catch logic errors at compile time:**

```viva
decreto edad: entero{0..150};           // Valid age range
decreto porcentaje: entero{0..100};     // Valid percentage
decreto indice: entero{0..len-1};       // Valid array index

edad = -5;        // COMPILE ERROR: -5 not in range
porcentaje = 200; // COMPILE ERROR: 200 > 100
```

### 4.3 Linear Resources
**Use exactly once, automatic cleanup:**

```viva
decreto archivo: recurso = abrir("data.txt");
leer(archivo);     // Consumes archivo
leer(archivo);     // COMPILE ERROR: already consumed
// No close() needed - compiler inserts cleanup
```

### 4.4 Pattern Matching
```viva
coincide resultado {
    Exito(valor) => usar(valor);
    Error(msg) => reportar(msg);
}
```

---

## Phase 5: Platform Expansion

### 5.1 Target Platforms
| Platform | Format | Status |
|----------|--------|--------|
| Linux x64 | ELF | ✅ Complete |
| Linux ARM64 | ELF | 🔜 Planned |
| macOS x64 | Mach-O | 🔜 Planned |
| macOS ARM64 | Mach-O | 🔜 Planned |
| Windows x64 | PE | 🔜 Planned |
| FreeBSD | ELF | 🔜 Planned |
| WebAssembly | WASM | 🔜 Planned |
| iOS | Mach-O | 🔜 Future |
| Android | ELF | 🔜 Future |

### 5.2 FFI (Foreign Function Interface)
**Use existing C libraries:**

```viva
externo "C" {
    cancion printf(fmt: *octeto, ...): entero;
    cancion malloc(size: entero): *octeto;
}
```

---

## What Becomes Possible

| After Phase | Can Build |
|-------------|-----------|
| **Now** | CLI tools, compilers, calculators, algorithms |
| **Phase 1** | Recursive algorithms, parsers, interpreters |
| **Phase 2** | Web servers, game engines, databases |
| **Phase 3** | Full applications, file processors, network tools |
| **Phase 4** | Provably correct software, safety-critical systems |
| **Phase 5** | Cross-platform apps, mobile, browser (WASM) |

---

## The Viva Difference

| Aspect | Viva | Rust | Go |
|--------|------|------|-----|
| Memory Safety | ✅ Compile-time | ✅ Compile-time | ✅ GC |
| Logic Errors | ✅ Refinement types | ❌ Runtime | ❌ Runtime |
| Learning Curve | Low (no annotations) | High (lifetimes) | Low |
| Compile Speed | Fast | Slow | Fast |
| Runtime Cost | Zero | Near-zero | GC pauses |
| Syntax | Spanish (readable) | English (complex) | English (simple) |

---

## Immediate Next Steps

```
Priority 1: Foundation
├── [ ] Fix recursion (unblocks algorithms)
├── [ ] Add structs (unblocks data modeling)
└── [ ] Add pointers (unblocks dynamic data)

Priority 2: Memory System
├── [ ] Basic malloc/free
├── [ ] Arena allocator
└── [ ] Ownership tracking

Priority 3: Real Apps
├── [ ] Strings
├── [ ] File I/O
└── [ ] Networking
```

---

## Philosophy

> "Simple enough for beginners, powerful enough for systems programming,
> safe enough for banks, fast enough for games."

Viva doesn't copy. Viva innovates:
- **Contextual Ownership** - Memory strategy from context, not annotations
- **Semantic Lifetimes** - Meaningful names, not `'a, 'b, 'c`
- **Refinement Types** - Catch logic errors, not just type errors
- **Effect Tracking** - Know what functions do at compile time
- **Spanish First** - Programming in your native language

**¡Viva Colombia!** 🇨🇴
