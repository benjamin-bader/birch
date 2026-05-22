# Config tooling plan

Priority-ordered checklist for making the config layer **correct**, **bounded**, and **safe to operate**. Higher items pay the most rent for a nascent daemon; lower items scale with public exposure, schema churn, and hostile inputs.

---

## Phase 1 — Boundaries and obvious correctness (do first)

### 1.1 Enforce dependency boundaries in Bazel

**Goal:** Config consumers never transitively depend on the TOML parser unless you intend it.

**Actions:**

- Split targets if needed: a **public** `cc_library` whose `hdrs` are only the façade (`IConfig`, `IConfigSource`, etc.), and an **implementation** library (or `implementation_deps` on the façade) that compiles `TomlConfigSource.cc` and links toml++.
- Set `visibility` on the parser-backed target so only `//config/...` and binaries that explicitly opt in can depend on it.
- Optional: add a tiny `cc_test` that includes only a “consumer” header and fails to compile if it accidentally pulls in `toml.hpp` (guard test pattern).

**Done when:** `bazel query` shows no path from `//server/...` (or chosen leaf) to toml++ unless you deliberately allow it.

### 1.2 Fix known semantic bugs in the runtime layer

**Goal:** `Reload` and lookups match mental model before tests encode the wrong behavior.

**Actions:**

- Replace `std::map::operator[]` in getters with non-inserting lookup (`find` / `at` / `optional`) so missing keys stay missing.
- On successful reload, **replace** the in-memory map (or clear then fill) so removed keys do not linger.
- Document snapshot vs live behavior for `IValueSource` (even one paragraph) until push/subscribe exists.

**Done when:** Unit tests for “missing key,” “key removed from file on reload,” and “failed reload preserves previous state” pass.

### 1.3 Table-driven unit tests for load and errors

**Goal:** Every bug fix gets a permanent row; reviewers see expected status and shape.

**Actions:**

- Extend `TomlConfigSourceTest` (and `Config` tests once meaningful) with cases: valid nested tables, invalid TOML, missing file, empty file, edge scalars you care about.
- Assert **error category** (`InvalidArgument`, etc.) and, where stable, parser diagnostics (line/column) if toml++ exposes them reliably.
- Keep golden `.toml` fixtures under `config/testdata/` (or similar) for readability.

**Done when:** New scalar/table behavior requires touching a fixture + one test row, not ad-hoc debug prints.

---

## Phase 2 — Operator experience (when the server reads a file)

### 2.1 `--check-config` (or equivalent)

**Goal:** Parse and validate without starting listeners; use in CI and support.

**Actions:**

- Add a mode (flag on the binary or small `//tools:check_config` target) that: resolves path, loads, logs summary (path, key count, timing), exits `0` on success and non-zero with a clear message on failure.
- Wire **example config** in-repo (`config/birch.example.toml` or `docs/examples/`) and run check against it in CI.

**Done when:** CI fails if the example config does not parse.

### 2.2 Structured logging on load

**Goal:** Production logs answer “what file” and “did reload succeed” without core dumps.

**Actions:**

- Log absolute path, outcome, and high-level stats after load/reload; redact or omit values that might be secrets (tokens, paths to keys).
- On failure, log status and first user-facing parse error string.

**Done when:** A bad deploy is diagnosable from logs alone.

---

## Phase 3 — CI hardening (sanitizers and static analysis)

### 3.1 Sanitizer build in CI

**Goal:** Catch UB, leaks, and use-after-free in custom parsing and string handling.

**Actions:**

- Add a Bazel `--config=asan` (and optionally `ubsan`) used in CI for `//config/...` tests and eventually server integration tests.
- Document local invocation in README or developer docs **only if** you already document builds; otherwise a comment in `.bazelrc` or `ci.yml` is enough.

**Done when:** CI runs at least one sanitizer configuration on Linux (macOS optional if cost is high).

### 3.2 Clang-tidy (incremental)

**Goal:** Mechanical consistency on code that will grow.

**Actions:**

- Enable a small preset (`bugprone-*`, `clang-analyzer-*`, portability checks relevant to parsing) on `//config/...` first, then widen.
- Fix or suppress with justification; avoid “enable everything” day-one churn.

**Done when:** Config sources are tidy-clean under the chosen preset.

---

## Phase 4 — Hostile and unknown inputs

### 4.1 Fuzz `LoadConfigValues` / parse entrypoint

**Goal:** The daemon does not crash or hang on arbitrary bytes in a config path.

**Actions:**

- Add a `cc_fuzz_test` (rules_fuzzing or Bazel’s native fuzzing) targeting the same code path as file load, feeding the parser buffer directly if possible.
- Seed corpus with valid minimal TOML, broken TOML, and empty input; add minimized crashes back to the corpus.

**Done when:** Fuzz job runs locally in a documented way; optionally OSS-Fuzz if the repo is public and you want continuous service.

---

## Phase 5 — Schema and scale (only when pain appears)

### 5.1 Single source of truth for “allowed config”

**Goal:** Required keys, types, and defaults are not scattered only across C++ conditionals.

**Actions:**

- Pick the lightest artifact that matches churn: **documented example + CI check** first; upgrade to **Cue / JSON Schema / custom checker** only when multiple binaries or frequent schema changes demand it.
- If you adopt an external schema, add one CI step: `schema validate examples/*.toml`.

**Done when:** Merging an invalid example config fails CI without running the full server.

### 5.2 Codegen from schema (optional, last)

**Goal:** Remove hand-written drift between schema and `struct ServerConfig`.

**Actions:**

- Only after (5.1) proves the schema is stable enough to justify tooling cost: generate structs and/or load glue, or generate schema from annotated structs—either direction is valid.

**Done when:** Adding a field is one schema (or annotation) change plus generated output review.

---

## Suggested implementation order (summary)

| Order | Item | Phase |
|------:|------|--------|
| 1 | Bazel visibility + `implementation_deps` / split targets | 1.1 |
| 2 | Fix `operator[]`, reload replace semantics, document snapshots | 1.2 |
| 3 | Fixtures + table-driven tests + reload tests | 1.3 |
| 4 | `--check-config` + example TOML in CI | 2.1 |
| 5 | Structured load/reload logging | 2.2 |
| 6 | ASan/UBSan CI for config (then server) | 3.1 |
| 7 | Clang-tidy on `//config/...` | 3.2 |
| 8 | Fuzz parse path + corpus | 4.1 |
| 9 | External schema validation in CI | 5.1 |
| 10 | Codegen from schema | 5.2 |

Reorder 4–5 relative to 6–7 based on whether the server starts reading config before you add sanitizer CI.

---

## Non-goals (for this plan)

- Replacing TOML or the parser library “for tooling.”
- Full observability stacks (metrics exporters) unless config reload becomes a first-class metric you need.
- Perfect diagnostic strings on day one; prioritize **stable error codes** and **one good example** first.
