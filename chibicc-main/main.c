#include "chibicc.h"
#include <stddef.h>
#include <stdint.h>

// Flag opsi (tanpa parsing argumen), ubah sesuai kebutuhan
bool opt_E = false;

// Ini versi dummy, karena kita fokus input buffer
static uint8_t *global_src_buf;
static size_t global_src_len;

// --------- NEW API: set source buffer ----------
void chibicc_set_source(uint8_t *buf, size_t len) {
    global_src_buf = buf;
    global_src_len = len;
}

// --------- MAIN PROCESS FUNCTION ---------
int chibicc_run_from_memory(char *out, size_t out_size, size_t *out_len) {
    // Tokenize dari memory buffer
    // (tokenize_input adalah fungsi yang kamu buat untuk tokenizing langsung dari buffer)
    Token *tok = tokenize_input((char*)global_src_buf, global_src_len);

    // Preprocess token list
    tok = preprocess(tok);

    // Kalau hanya ingin preprocessor output saja
    if (opt_E) {
        // print preprocessed tokens atau simpan ke buffer
        // mis. print_tokens(tok);
        return 0;
    }

    // Parse jadi AST
    Obj *prog = parse(tok);

    // Codegen ke internal buffer
    char *asm_buf;
    size_t asm_size;
    MemStream *outbuf = memstream_open();
    codegen(prog, outbuf);
    memstream_flush(outbuf, &asm_buf, &asm_size);
    memstream_close(outbuf);
    
    // Copy assembly to output buffer
    if (out && out_size > 0 && asm_size > 0) {
        size_t copy_size = (asm_size < out_size) ? asm_size : out_size - 1;
        for (size_t i = 0; i < copy_size; i++) {
            out[i] = asm_buf[i];
        }
        if (out_size > asm_size) {
            out[asm_size] = '\0';
        }
    }
    
    if (out_len) *out_len = asm_size;

    // Output buffer asm_buf/asm_size sekarang berisi assembly
    // Kembalikan atau simpan sesuai kebutuhan
    // free(asm_buf) nanti jika tidak perlu lagi

    return 0;
}
