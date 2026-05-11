#include <elf.h>
#include <string.h>
#include <stdlib.h>
#include "GenType.h"

static constexpr const char *STANDARD_LOADER = "/lib64/ld-linux-x86-64.so.2";
static constexpr const char *STANDARD_LIB    = "libc.so.6";
static constexpr        int  STANDARD_HASH_SIZE = 32;
static constexpr        int  SIZE_PLT_STAB      = 16;
static constexpr   uint64_t  PAGE_SIZE      = 0x1000;
static constexpr   uint64_t  DYNAMIC_AMOUNT = 11;

static void fill_interp(KTL_ElfContext *cont) {
    assert(cont);

    int size = strlen(STANDARD_LOADER);
    cont->interp.data.bytes = (uint8_t *)calloc(size + 1, sizeof(uint8_t));
    if (cont->interp.data.bytes == NULL)    ExitF("NULL calloc", );

    memcpy(cont->interp.data.bytes, STANDARD_LOADER, size);
    cont->interp.align    = 0;
    cont->interp.file_off = cont->phdr_amount * sizeof(Elf64_Phdr) + sizeof(Elf64_Ehdr);
    cont->interp.vaddr    = cont->virt_adr;
    cont->interp.size     = size + 1;

    return ;
}

static void fill_import_symbol(KTL_ElfContext *cont) {
    assert(cont);

    int got_count   = 4;
    int plt_count   = 0;
    int dynamic_off = 1;
    int dynstr_size = 1;

    KTL_LabelFix_Map *fix_map = cont->gen_cont->file_outside_fix_map;

    cont->import.imps = (KTL_ElfImport *)calloc(fix_map->size, sizeof(KTL_ElfImport));
    if (cont->import.imps == NULL)  ExitF("NULL calloc", );
    cont->import.size = 0;

    for (int i = 0; i < fix_map->size; i++) {
        KTL_LabelFix_Entry *fix_e = fix_map->data + i;
        KTL_ElfImport      *imp   = find_import(cont, fix_e->target);

        if (imp == NULL) {
            imp = cont->import.imps + cont->import.size;
            cont->import.size++;
            imp->kind = fix_e->kind;

            imp->got_idx = got_count++;
            imp->plt_idx = plt_count++;
            imp->name    = fix_e->target;

            dynstr_size += strlen(imp->name);
        }
    }

    uint64_t hash_size    = get_size_hash(cont);
    cont->dynsym.file_off = cont->interp.file_off + cont->interp.size + hash_size;
    cont->dynsym.vaddr    = cont->virt_adr + cont->interp.size + hash_size;

    KTL_GenFlat *dynsym = &cont->dynsym.data;
    dynsym->bytes = (uint8_t *)calloc(cont->import.size + 1, sizeof(Elf64_Sym));
    if (dynsym->bytes == NULL)    ExitF("NULL calloc", );
    dynsym->len = (cont->import.size + 1) * sizeof(Elf64_Sym);

    Elf64_Sym *syms = (Elf64_Sym *)dynsym->bytes;

    uint64_t dynsym_size  = (cont->import.size + 1) * sizeof(Elf64_Sym);
    cont->dynstr.file_off = cont->dynsym.file_off + dynsym_size;
    cont->dynstr.vaddr    = cont->dynsym.vaddr + dynsym_size;

    KTL_GenFlat *dynstr = &cont->dynstr.data;
    dynstr->bytes = (uint8_t *)calloc(dynstr_size, sizeof(uint8_t));
    if (dynstr->bytes == NULL)      ExitF("NULL calloc", );
    dynstr->len = dynstr_size;

    int pos = 1;
    memcpy(dynstr->bytes + pos, STANDARD_LIB, strlen(STANDARD_LIB));
    pos += strlen(STANDARD_LIB);

    syms[STN_UNDEF] = {};
    for (int i = 0; i < cont->import.size; i++) {
        int len = strlen(cont->import.imps[i].name);
        memcpy(dynstr->bytes + pos, cont->import.imps[i].name, len);

        syms[i + 1].st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
        syms[i + 1].st_shndx = SHN_UNDEF;
        syms[i + 1].st_size  = 0;
        syms[i + 1].st_value = 0;
        syms[i + 1].st_name  = pos;
        syms[i + 1].st_other = STV_DEFAULT;

        cont->import.imps[i].dynstr_offset = pos;
        cont->import.imps[i].dynsym_offset = i + 1;

        pos += len;
    }
    return ;
}

static void fill_symbols(KTL_ElfContext *cont) {
    assert(cont);

    KTL_LabelFix_Map *fix_map = cont->gen_cont->file_outside_fix_map;
    for (int i = 0; i < fix_map->size; i++) {
        KTL_LabelFix_Entry *fix = fix_map->data + i;

        KTL_

    }

}

static uint64_t get_size_hash(KTL_ElfContext *cont) {
    assert(cont);
    return (2 + STANDARD_HASH_SIZE + cont->import.size + 1) * 4;
}

uint32_t elf_hash(const uint8_t *name) {
    uint32_t h = 0, g = 0, i = 0;
    while (name[i]) {
        h = (h << 4) + name[i++];
        g = h & 0xf'000'0000;
        if (g) {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h;
}

static void fill_hash(KTL_ElfContext *cont) {
    assert(cont);

    Elf64_Sym *syms = (Elf64_Sym *)cont->dynsym.data.bytes;

    int nbucket = STANDARD_HASH_SIZE;
    int nchain  = cont->import.size + 1;

    cont->hash.bytes = (uint8_t *)calloc(2 + nbucket + nchain, sizeof(uint32_t));
    if (cont->hash.bytes == NULL)   ExitF("NULL calloc", );
    uint32_t *hash = (uint32_t *)cont->hash.bytes;

    hash[0] = nbucket;
    hash[1] = nchain;
    uint32_t *buckets = hash + 2;
    uint32_t *chains  = buckets + nbucket;

    for (int i = 1; i < nchain; i++) {
        if (ELF32_ST_BIND(syms[i].st_info) == STB_LOCAL) {
            chains[i] = 0;
            continue;
        }
        const uint8_t *name = cont->dynstr.data.bytes + syms[i].st_name;
        uint32_t h          = elf_hash(name) % nbucket;
        chains[i]           = buckets[h];
        buckets[h]          = i;
    }
    cont->hash.file_off = cont->interp.file_off + cont->interp.size;
    cont->hash.vaddr    = cont->interp.vaddr    + cont->interp.size;
    cont->hash.size     = get_size_hash(cont);

    return ;
}

static void fill_layout_context(KTL_ElfContext *cont) {
    assert(cont);

    cont->rela_plt.file_off = cont->dynstr.file_off + cont->dynstr.size;
    cont->rela_plt.vaddr    = cont->dynstr.vaddr    + cont->dynstr.size;
    cont->rela_plt.size     = get_rela_plt_size(cont);

    cont->plt.file_off = cont->rela_plt.file_off + cont->rela_plt.size;
    cont->plt.vaddr    = cont->rela_plt.vaddr    + cont->rela_plt.size;
    cont->plt.size     = get_plt_size(cont);

    cont->text.file_off = cont->plt.file_off + cont->plt.size;
    cont->text.vaddr    = cont->plt.vaddr    + cont->plt.size;
    cont->text.size     = cont->gen_cont->out_flat.text.len;
    cont->text.data     = cont->gen_cont->out_flat.text;

    cont->rodata.file_off = cont->text.file_off + cont->text.size;
    cont->rodata.vaddr    = cont->text.vaddr    + cont->text.size;
    cont->rodata.size     = cont->gen_cont->out_flat.rodata.len;
    cont->rodata.data     = cont->gen_cont->out_flat.rodata;

    cont->dynamic.file_off = align_up_u64(cont->rodata.file_off + cont->rodata.size, PAGE_SIZE);
    cont->dynamic.vaddr    = align_up_u64(cont->rodata.vaddr + cont->rodata.size, PAGE_SIZE);
    cont->dynamic.size     = get_dynamic_size(cont);

    cont->got_plt.file_off = cont->dynamic.file_off + cont->dynamic.size;
    cont->got_plt.vaddr    = cont->dynamic.vaddr    + cont->dynamic.size;
    cont->got_plt.size     = get_got_plt_size(cont);

    cont->data.file_off = cont->got_plt.file_off + cont->got_plt.size;
    cont->data.vaddr    = cont->got_plt.vaddr    + cont->got_plt.size;
    cont->data.size     = cont->gen_cont->out_flat.data.len;

    return ;
}

static uint64_t align_up_u64(uint64_t val, uint64_t alg) {
    return (val + alg - 1) & ~(alg - 1);
}

static void fill_rela_plt(KTL_ElfContext *cont) {
    assert(cont);

    int count = cont->import.size;

    cont->rela_plt.data.bytes = (uint8_t *)calloc(count, sizeof(Elf64_Rela));
    if (!cont->rela_plt.data.bytes)     ExitF("NULL calloc", );

    cont->rela_plt.data.len = count * sizeof(Elf64_Rela);

    Elf64_Rela *rela = (Elf64_Rela *)cont->rela_plt.data.bytes;

    for (int i = 0; i < count; i++) {

        KTL_ElfImport *imp = &cont->import.imps[i];

        rela[i].r_offset = cont->got_plt.vaddr + (3 + i) * sizeof(uint64_t);
        rela[i].r_info   = ELF64_R_INFO( imp->dynsym_offset, R_X86_64_JUMP_SLOT);
        rela[i].r_addend = 0;
    }
    return ;
}

static void fill_plt(KTL_ElfContext *cont) {
    assert(cont);

    int count = cont->import.size;
    int size = (count + 1) * SIZE_PLT_STAB;

    cont->plt.data.bytes = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (!cont->plt.data.bytes) ExitF("NULL calloc", );

    cont->plt.data.len = size;
    uint8_t *plt = cont->plt.data.bytes;

    /* PLT 0 */
    uint8_t plt0[] = {
        0xff, 0x35, 0,0,0,0,
        0xff, 0x25, 0,0,0,0,
        0x0f, 0x1f, 0x40, 0x00 };
    memcpy(plt, plt0, sizeof(plt0));

    int32_t disp_push = (int32_t)( (cont->got_plt.vaddr + 8) - (cont->plt.vaddr + 6) );
    memcpy(plt + 2, &disp_push, 4);

    int32_t disp_jmp = (int32_t)( (cont->got_plt.vaddr + 16) - (cont->plt.vaddr + 12));
    memcpy(plt + 8, &disp_jmp, 4);

    /* PLT N */
    for (int i = 0; i < count; i++) {

        uint8_t *entry = plt + SIZE_PLT_STAB * (i + 1);

        uint64_t entry_vaddr = cont->plt.vaddr + SIZE_PLT_STAB * (i + 1);

        entry[0] = 0xff;
        entry[1] = 0x25;

        uint64_t got_slot = cont->got_plt.vaddr + (3 + i) * sizeof(uint64_t);
        int32_t disp = (int32_t)( got_slot - (entry_vaddr + 6) );
        memcpy(entry + 2, &disp, 4);

        entry[6] = 0x68;
        uint32_t reloc = i;
        memcpy(entry + 7, &reloc, 4);

        entry[11] = 0xe9;
        int32_t rel32 = (int32_t)(cont->plt.vaddr - (entry_vaddr + 16) );

        memcpy(entry + 12, &rel32, 4);
    }
    return ;
}

static void fill_text(KTL_ElfContext *cont) {
    assert(cont);

    cont->text.data = cont->gen_cont->out_flat.text;
}

static void fill_rodata(KTL_ElfContext *cont) {
    assert(cont);

    cont->rodata.data = cont->gen_cont->out_flat.rodata;
}

static void fill_data(KTL_ElfContext *cont) {
    assert(cont);

    cont->data.data = cont->gen_cont->out_flat.data;
}

static int get_plt_size(KTL_ElfContext *cont) {
    assert(cont);
    return cont->import.size * SIZE_PLT_STAB;
}

static int get_rela_plt_size(KTL_ElfContext *cont) {
    assert(cont);
    return cont->import.size * sizeof(Elf64_Rela);
}

static int get_dynamic_size(KTL_ElfContext *cont) {
    assert(cont);
    (void) cont;
    return DYNAMIC_AMOUNT * sizeof(Elf64_Dyn);
}

static int get_got_plt_size(KTL_ElfContext *cont) {
    assert(cont);

    return (3 + cont->import.size) * 8;
}

static void fill_got_plt(KTL_ElfContext *cont) {
    assert(cont);

    cont->got_plt.data.bytes = (uint8_t *)calloc(get_got_plt_size(cont), sizeof(uint8_t));
    if (cont->got_plt.data.bytes == NULL)   ExitF("NULL calloc", );
    cont->got_plt.data.len  = get_got_plt_size(cont);


    uint64_t *data = (uint64_t *)cont->got_plt.data.bytes;
    data[0] = cont->dynamic.vaddr;
    data[1] = 0;
    data[2] = 0;

    int pos = 3;

    for (int i = 0; i < cont->import.size; i++) {
        data[pos++] = cont->plt.vaddr + (i + 1) * 8 + 6;
    }
    return ;
}

static void fill_dynamic(KTL_ElfContext *cont) {
    assert(cont);

    Elf64_Dyn *dyn = (Elf64_Dyn *)calloc(11, sizeof(Elf64_Dyn));
    if (dyn == NULL)    ExitF("NULL calloc", );

    int i = 0;

    dyn[i].d_tag        = DT_NEEDED;
    dyn[i++].d_un.d_val = 1;

    dyn[i].d_tag        = DT_HASH;
    dyn[i++].d_un.d_ptr = cont->hash.vaddr;

    dyn[i].d_tag        = DT_STRTAB;
    dyn[i++].d_un.d_ptr = cont->dynstr.vaddr;

    dyn[i].d_tag        = DT_SYMTAB;
    dyn[i++].d_un.d_ptr = cont->dynsym.vaddr;

    dyn[i].d_tag        = DT_STRSZ;
    dyn[i++].d_un.d_val = cont->dynstr.size;

    dyn[i].d_tag        = DT_SYMENT;
    dyn[i++].d_un.d_val = sizeof(Elf64_Sym);

    dyn[i].d_tag        = DT_PLTGOT;
    dyn[i++].d_un.d_ptr = cont->got_plt.vaddr;

    dyn[i].d_tag        = DT_JMPREL;
    dyn[i++].d_un.d_ptr = cont->rela_plt.vaddr;

    dyn[i].d_tag        = DT_PLTRELSZ;
    dyn[i++].d_un.d_val = cont->rela_plt.size;

    dyn[i].d_tag        = DT_PLTREL;
    dyn[i++].d_un.d_val = DT_RELA;

    dyn[i++] = (Elf64_Dyn) { .d_tag = DT_NULL };

    cont->dynamic.data.bytes = (uint8_t *)dyn;
    cont->dynamic.data.len   = i * sizeof(Elf64_Dyn);
    cont->dynamic.align = 8;

    return ;
}

static KTL_ElfImport *find_import(KTL_ElfContext *cont, KTL_StrID name) {
    assert(cont);
    assert(StrIDCheck(name));

    for (int i = 0; i < cont->import.size; i++) {
        if (cont->import.imps[i].name == name)  return cont->import.imps + i;
    }

    return NULL;
}

static void emit_elf_header(KTL_ElfContext *cont) {
    assert(cont);

    Elf64_Ehdr hdr = {};

    memcpy(hdr.e_ident, ELFMAG, SELFMAG);

    hdr.e_ident[EI_CLASS]   = ELFCLASS64;
    hdr.e_ident[EI_DATA]    = ELFDATA2LSB;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    hdr.e_ident[EI_OSABI]   = ELFOSABI_SYSV;

    hdr.e_type      = ET_EXEC;
    hdr.e_machine   = EM_X86_64;
    hdr.e_version   = EV_CURRENT;

    hdr.e_entry     = cont->text.vaddr;
    hdr.e_phoff     = sizeof(Elf64_Ehdr);
    hdr.e_ehsize    = sizeof(Elf64_Ehdr);
    hdr.e_phentsize = sizeof(Elf64_Phdr);
    hdr.e_phnum     = 6;

    fwrite(&hdr, sizeof(hdr), 1, cont->stream);
}

static void emit_elf_phdr(KTL_ElfContext *cont) {
    assert(cont);

    Elf64_Phdr ph[6] = {};

    ph[0].p_type   = PT_PHDR;
    ph[0].p_offset = sizeof(Elf64_Ehdr);

    ph[0].p_vaddr = cont->virt_adr + sizeof(Elf64_Ehdr);
    ph[0].p_paddr = ph[0].p_vaddr;

    ph[0].p_filesz = 6 * sizeof(Elf64_Phdr);
    ph[0].p_memsz = ph[0].p_filesz;

    ph[0].p_flags = PF_R;
    ph[0].p_align = 8;

    ph[1].p_type   = PT_INTERP;
    ph[1].p_offset = cont->interp.file_off;

    ph[1].p_vaddr  = cont->interp.vaddr;
    ph[1].p_paddr  = cont->interp.vaddr;

    ph[1].p_filesz = cont->interp.size;
    ph[1].p_memsz  = cont->interp.size;

    ph[1].p_flags  = PF_R;
    ph[1].p_align  = 1;

    ph[2].p_type   = PT_LOAD;
    ph[2].p_offset = 0;

    ph[2].p_vaddr  = cont->virt_adr;
    ph[2].p_paddr  = cont->virt_adr;

    ph[2].p_filesz = cont->rodata.file_off + cont->rodata.size;
    ph[2].p_memsz  = ph[2].p_filesz;

    ph[2].p_flags  = PF_R | PF_X;
    ph[2].p_align  = PAGE_SIZE;


    ph[3].p_type   = PT_LOAD;
    ph[3].p_offset = cont->dynamic.file_off;

    ph[3].p_vaddr  = cont->dynamic.vaddr;
    ph[3].p_paddr  = cont->dynamic.vaddr;

    ph[3].p_filesz = cont->dynamic.file_off + cont->dynamic.size - cont->got_plt.file_off;
    ph[3].p_memsz = ph[3].p_filesz;

    ph[3].p_flags = PF_R | PF_W;
    ph[3].p_align = PAGE_SIZE;

    ph[4].p_type   = PT_DYNAMIC;
    ph[4].p_offset = cont->dynamic.file_off;

    ph[4].p_vaddr  = cont->dynamic.vaddr;
    ph[4].p_paddr  = cont->dynamic.vaddr;

    ph[4].p_filesz = cont->dynamic.size;
    ph[4].p_memsz  = cont->dynamic.size;

    ph[4].p_flags  = PF_R | PF_W;
    ph[4].p_align  = 8;

    ph[5].p_type  = PT_GNU_STACK;
    ph[5].p_flags = PF_R | PF_W;

    fwrite(ph, sizeof(ph), 1, cont->stream);
}

static void write_to_file(KTL_ElfContext *cont) {
    assert(cont);
    FILE *stream = cont->stream;

    fwrite(cont->interp.data.bytes,   1, cont->interp.size,       stream);
    fwrite(cont->hash.data.bytes,     1, cont->hash.size,         stream);
    fwrite(cont->dynsym.data.bytes,   1, cont->dynsym.data.len,   stream);
    fwrite(cont->dynstr.data.bytes,   1, cont->dynstr.data.len,   stream);
    fwrite(cont->rela_plt.data.bytes, 1, cont->rela_plt.data.len, stream);
    fwrite(cont->plt.data.bytes,      1, cont->plt.data.len,      stream);
    fwrite(cont->text.data.bytes,     1, cont->text.size,         stream);
    fwrite(cont->rodata.data.bytes,   1, cont->rodata.size,       stream);

    long cur      = ftell(stream);
    long expected = (long)cont->rodata.file_off + cont->rodata.size;
    long target   = (long)cont->dynamic.file_off;
    if (cur < target) {
        long gap = target - cur;
        uint8_t *zeros = (uint8_t *)calloc(gap, 1);
        fwrite(zeros, 1, gap, stream);
        free(zeros);
    }
    fseek(stream, target, SEEK_SET);  // на всякий случай

    fwrite(cont->dynamic.data.bytes, 1, cont->dynamic.data.len, stream);
    fwrite(cont->got_plt.data.bytes, 1, cont->got_plt.data.len, stream);
    fwrite(cont->data.data.bytes,    1, cont->data.size,        stream);

    return ;
}
