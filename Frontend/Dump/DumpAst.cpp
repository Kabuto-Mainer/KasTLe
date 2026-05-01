#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "DumpAstType.h"
#include "ASTCommon.h"
#include "StrMap.h"
#include "TypeMap.h"
#include "SymMap.h"

// =======================================================================
// CONSTANTS
// =======================================================================
constexpr static int KTL_DUMP_AUTO_OPEN_DEPTH = 3;
constexpr static int KTL_DUMP_SYMS_INIT_CAP   = 64;

enum KTL_DumpStage {
    KTL_STAGE_EMPTY,
    KTL_STAGE_PARSE,        /* есть raw VAR/CALL                       */
    KTL_STAGE_RESOLVED,     /* всё резолвлено, но не все expr типизировны */
    KTL_STAGE_ANALYZED,     /* всё типизировано (с возможными ошибками)  */
};

// =======================================================================
// HELPER FUNCTIONS DECLARATION
// =======================================================================
static void ktl_dump_header     (KTL_DumpAstContext *cont);
static void ktl_dump_footer     (KTL_DumpAstContext *cont);

static void ktl_dump_node       (KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node,
                                 int                 depth);
static void ktl_dump_children   (KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node,
                                 int                 depth);
static void ktl_dump_summary    (KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node);
static void ktl_dump_pos        (KTL_DumpAstContext *cont,
                                 KTL_SourcePos       pos);
static void ktl_dump_extra_attrs(KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node);
static void ktl_dump_type_annot (KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node);

/* sym/type tables / status */
static void ktl_dump_pipeline   (KTL_DumpAstContext *cont);
static void ktl_dump_global_syms(KTL_DumpAstContext *cont);
static void ktl_dump_types_table(KTL_DumpAstContext *cont);
static void ktl_dump_sym_table  (KTL_DumpAstContext *cont,
                                 KTL_SymbolMap      *map);
static void ktl_dump_scope_syms (KTL_DumpAstContext *cont,
                                 KTL_AstNode        *node);
static void ktl_dump_mods       (FILE *s, int mod);

/* pre-pass: registers symbols and counts statistics */
static int  ktl_sym_index       (KTL_DumpAstContext *cont, KTL_SymbolEntry *e);
static void ktl_sym_register    (KTL_DumpAstContext *cont, KTL_SymbolEntry *e);
static void ktl_register_map    (KTL_DumpAstContext *cont, KTL_SymbolMap   *map);
static void ktl_pre_walk        (KTL_DumpAstContext *cont, KTL_AstNode     *node);

/* type access — "is expression with a type" */
static bool ktl_get_expr_type      (KTL_AstNode *node, KTL_TypeID *out);
static bool ktl_should_show_annot  (KTL_AstNode *node);
static KTL_DumpStage ktl_detect_stage(KTL_DumpStats *st);

/* small */
static void        ktl_html_escape (FILE *stream, const char *s);
static const char *ktl_kind_name   (KTL_AstNodeKind kind);
static const char *ktl_oper_symbol (KTL_Oper        op);
static void        ktl_type_print  (FILE         *stream,
                                    KTL_TypeMap  *map,
                                    KTL_TypeID    id);

static inline const char *str_or(KTL_StrID id, const char *fallback) {
    return id ? id : fallback;
}

// =======================================================================
// API
// =======================================================================
void KTL_AstDumpRaw(KTL_AstNode   *root,
                    KTL_StrMap    *str_map,
                    KTL_SymbolMap *global_map,
                    KTL_TypeMap   *type_map,
                    const char    *file) {
    assert(root);
    assert(str_map);
    assert(global_map);
    assert(type_map);
    assert(file);

    KTL_DumpAstContext cont = {};
    cont.global_map = global_map;
    cont.str_map    = str_map;
    cont.type_map   = type_map;
    cont.stream     = fopen(file, "wb");
    if (cont.stream == NULL)    ExitF("NULL File", );

    /* Реестр символов */
    cont.all_syms = (KTL_SymbolEntry **)calloc(KTL_DUMP_SYMS_INIT_CAP,
                                               sizeof(KTL_SymbolEntry *));
    if (cont.all_syms == NULL)  ExitF("NULL Calloc", );
    cont.cap_syms = KTL_DUMP_SYMS_INIT_CAP;
    cont.n_syms   = 0;

    /* 1. Пред-проход: символы + статистика. */
    ktl_register_map(&cont, global_map);
    ktl_pre_walk(&cont, root);

    cont.stage = (int) ktl_detect_stage(&cont.stats);

    /* 2. Печатаем. */
    ktl_dump_header(&cont);

    ktl_dump_pipeline   (&cont);
    ktl_dump_global_syms(&cont);
    ktl_dump_types_table(&cont);

    ktl_dump_node(&cont, root, 0);

    ktl_dump_footer(&cont);

    free(cont.all_syms);
    fclose(cont.stream);
}

// =======================================================================
// STAGE / STATS
// =======================================================================
static bool ktl_get_expr_type(KTL_AstNode *node, KTL_TypeID *out) {
    assert(out);
    *out = KTL_BAD_TYPE_ID;
    if (node == NULL)   return false;

    switch (node->kind) {
        case KTL_AST_VALUE_INT:
            *out = node->data.int_val.type_res;
            return true;
        case KTL_AST_VALUE_STR:
            *out = node->data.str_val.type_res;
            return true;
        case KTL_AST_BINARY_OPER:
        case KTL_AST_UNARY_OPER:
            *out = node->data.oper.type_res;
            return true;
        case KTL_AST_INDEX_ACCESS:
            *out = node->data.index.type_value;
            return true;
        case KTL_AST_FIELD_ACCESS:
            *out = node->data.field.type;
            return true;
        case KTL_AST_CAST:
            *out = node->data.cast.target;
            return true;
        case KTL_AST_VARIABLE:
            if (!node->data.var.is_raw &&
                node->data.var.info.res.entry != NULL) {
                *out = node->data.var.info.res.entry->var.type;
            }
            return true;
        case KTL_AST_FUNCTION_CALL:
            if (!node->data.func_call.is_raw &&
                node->data.func_call.info.res.entry != NULL) {
                *out = node->data.func_call.info.res.entry->func.ret_type;
            }
            return true;
        default:
            return false;
    }
}

/* CAST уже печатает свой target в summary — не дублируем. */
static bool ktl_should_show_annot(KTL_AstNode *node) {
    if (node == NULL)               return false;
    if (node->kind == KTL_AST_CAST) return false;

    KTL_TypeID t;
    return ktl_get_expr_type(node, &t);
}

static KTL_DumpStage ktl_detect_stage(KTL_DumpStats *st) {
    assert(st);

    if (st->total == 0)                                 return KTL_STAGE_EMPTY;
    if (st->raw_vars + st->raw_calls > 0)               return KTL_STAGE_PARSE;
    if (st->exprs_total == 0)                           return KTL_STAGE_PARSE;
    if (st->exprs_typed < st->exprs_total)              return KTL_STAGE_RESOLVED;
    return KTL_STAGE_ANALYZED;
}

static const char *ktl_stage_name(KTL_DumpStage st) {
    switch (st) {
        case KTL_STAGE_EMPTY:    return "Empty AST";
        case KTL_STAGE_PARSE:    return "Parse";
        case KTL_STAGE_RESOLVED: return "Resolved";
        case KTL_STAGE_ANALYZED: return "Analyzed";
    }
    return "?";
}

// =======================================================================
// HEADER / FOOTER
// =======================================================================
static void ktl_dump_header(KTL_DumpAstContext *cont) {
    assert(cont);

    fprintf(cont->stream,
"<!DOCTYPE html>\n"
"<html lang=\"en\"><head><meta charset=\"utf-8\"><title>AST dump</title>\n"
"<style>\n"
"  :root {\n"
"    --bg:        #1e1e1e;\n"
"    --bg2:       #252525;\n"
"    --bg3:       #2a2a2a;\n"
"    --fg:        #d4d4d4;\n"
"    --muted:     #7a7a7a;\n"
"    --accent:    #4ec9b0;\n"
"    --decl:      #569cd6;\n"
"    --ctrl:      #c586c0;\n"
"    --expr:      #dcdcaa;\n"
"    --leaf:      #9cdcfe;\n"
"    --block:     #808080;\n"
"    --bad:       #f48771;\n"
"    --ok:        #6abf69;\n"
"    --warn:      #d7ba7d;\n"
"    --hl:        rgba(78, 201, 176, 0.22);\n"
"  }\n"
"  body { background: var(--bg); color: var(--fg);\n"
"         font-family: 'JetBrains Mono','Cascadia Mono',monospace;\n"
"         font-size: 13px; padding: 16px; margin: 0; }\n"
"  .toolbar { position: sticky; top: 0; background: var(--bg);\n"
"             padding: 8px 0; border-bottom: 1px solid #333;\n"
"             margin-bottom: 8px; z-index: 10; display: flex;\n"
"             gap: 6px; align-items: center; flex-wrap: wrap; }\n"
"  .toolbar button { background: var(--bg3); color: var(--fg);\n"
"                    border: 1px solid #444; padding: 4px 10px;\n"
"                    cursor: pointer; font-family: inherit; font-size: 12px; }\n"
"  .toolbar button:hover { background: #3a3a3a; }\n"
"  .toolbar label { font-size: 12px; color: var(--muted);\n"
"                   user-select: none; cursor: pointer;\n"
"                   margin-left: 8px; }\n"
"  .toolbar label input { vertical-align: middle; margin-right: 3px; }\n"
"  .toolbar .stage-badge { margin-left: auto; padding: 2px 10px;\n"
"                          border-radius: 3px; background: var(--bg3);\n"
"                          font-size: 12px; }\n"
"  .toolbar .stage-badge.parse    { color: var(--warn); }\n"
"  .toolbar .stage-badge.resolved { color: var(--decl); }\n"
"  .toolbar .stage-badge.analyzed { color: var(--ok); }\n"
"  .toolbar .stage-badge.errors   { color: var(--bad);\n"
"                                   border: 1px solid var(--bad); }\n"
"\n"
"  details { margin-left: 14px; border-left: 1px solid #2f2f2f;\n"
"            padding-left: 8px; }\n"
"  details > summary { cursor: pointer; padding: 2px 4px;\n"
"                      list-style: none; }\n"
"  details > summary::-webkit-details-marker { display: none; }\n"
"  details > summary::before { content: '\\25B8\\00A0'; color: var(--muted); }\n"
"  details[open] > summary::before { content: '\\25BE\\00A0'; }\n"
"  .node { margin: 1px 0; }\n"
"  .leaf { margin-left: 14px; padding: 2px 4px 2px 22px;\n"
"          border-left: 1px solid #2f2f2f; }\n"
"  .kind { color: var(--muted); margin-right: 6px; font-size: 11px; }\n"
"  .name { color: var(--accent); }\n"
"  .type { color: #ce9178; }\n"
"  .op   { color: var(--expr); font-weight: bold; }\n"
"  .lit  { color: #b5cea8; }\n"
"  .str  { color: #ce9178; }\n"
"  .raw  { color: var(--warn); font-style: italic; font-size: 10px;\n"
"          margin-left: 6px; }\n"
"  .pos  { color: var(--muted); float: right; font-size: 11px; }\n"
"\n"
"  .annot { color: var(--accent); opacity: 0.65;\n"
"           margin-left: 8px; font-size: 11px; font-weight: normal; }\n"
"  .annot .type { color: var(--accent); }\n"
"  .annot.bad { color: var(--bad); opacity: 1; font-weight: bold; }\n"
"  body.hide-annot .annot { display: none; }\n"
"\n"
"  /* sections */\n"
"  details.section { margin: 0 0 12px 0; border-left: 3px solid var(--decl);\n"
"                    padding-left: 10px; background: var(--bg2); }\n"
"  details.section > summary { font-weight: bold; padding: 6px 4px;\n"
"                              color: var(--decl); }\n"
"  details.section.pipeline { border-left-color: var(--ok); }\n"
"  details.section.pipeline > summary { color: var(--ok); }\n"
"  details.scope-syms { margin: 2px 0 4px 14px; border-left-color: #444; }\n"
"  details.scope-syms > summary { color: var(--muted); font-size: 11px;\n"
"                                  font-style: italic; }\n"
"  table.sym-table { border-collapse: collapse; margin: 4px 0 8px 14px;\n"
"                    font-size: 12px; }\n"
"  table.sym-table th, table.sym-table td {\n"
"     border: 1px solid #333; padding: 2px 10px; text-align: left;\n"
"     vertical-align: top; }\n"
"  table.sym-table th { background: var(--bg3); color: var(--muted);\n"
"                       font-weight: normal; font-size: 11px; }\n"
"  table.sym-table tbody tr { cursor: pointer; }\n"
"  table.sym-table tbody tr:hover { background: var(--bg3); }\n"
"  table.sym-table td.idx { color: var(--muted); text-align: right; }\n"
"  .empty { color: var(--muted); font-style: italic;\n"
"           padding: 4px 0 8px 14px; }\n"
"  h4.subhead { color: var(--muted); font-weight: normal;\n"
"               margin: 8px 0 2px 14px; font-size: 12px; }\n"
"\n"
"  /* pipeline status grid */\n"
"  .stat-grid { display: grid; grid-template-columns: auto 1fr;\n"
"               gap: 4px 18px; margin: 6px 0 6px 14px;\n"
"               font-size: 12px; }\n"
"  .stat-grid .label { color: var(--muted); }\n"
"  .stat-grid .ok    { color: var(--ok); }\n"
"  .stat-grid .bad   { color: var(--bad); }\n"
"  .stat-grid .warn  { color: var(--warn); }\n"
"\n"
"  .hl-sym, table.sym-table tbody tr.hl-sym {\n"
"     background: var(--hl) !important;\n"
"     outline: 1px solid var(--accent); }\n"
"\n"
"  .node-FUNCTION_DECL > summary,\n"
"  .node-VARIABLE_DECL,\n"
"  .node-TYPEDEF      > summary,\n"
"  .node-STRUCT_DECL  > summary { border-left-color: var(--decl); }\n"
"  .node-IF_BRANCH    > summary,\n"
"  .node-ELSE_BRANCH  > summary,\n"
"  .node-WHILE_BLOCK  > summary,\n"
"  .node-FOR_BLOCK    > summary,\n"
"  .node-COND_BLOCK   > summary { border-left-color: var(--ctrl); }\n"
"  .node-BLOCK        > summary { color: var(--block); }\n"
"  [data-typed=\"bad\"]  > summary,\n"
"  [data-typed=\"bad\"].leaf { background: rgba(244,135,113,0.07); }\n"
"</style>\n"
"</head>\n"
"<body>\n");

    /* Toolbar */
    KTL_DumpStage   stage     = (KTL_DumpStage) cont->stage;
    bool            has_errs  = (stage == KTL_STAGE_ANALYZED) &&
                                 cont->stats.bad_types > 0;
    const char     *stage_cls = "parse";
    if      (stage == KTL_STAGE_RESOLVED)   stage_cls = "resolved";
    else if (stage == KTL_STAGE_ANALYZED)   stage_cls = "analyzed";

    fprintf(cont->stream,
"<div class=\"toolbar\">\n"
"  <button onclick=\"document.querySelectorAll('details').forEach(d=>d.open=true)\">Expand all</button>\n"
"  <button onclick=\"document.querySelectorAll('details').forEach(d=>d.open=false)\">Collapse all</button>\n"
"  <button onclick=\"document.querySelectorAll('.hl-sym').forEach(x=>x.classList.remove('hl-sym'))\">Clear highlight</button>\n"
"  <label><input type=\"checkbox\" id=\"show-types\" checked> Show types</label>\n"
"  <span class=\"stage-badge %s%s\">%s%s</span>\n"
"</div>\n",
        stage_cls, has_errs ? " errors" : "",
        ktl_stage_name(stage),
        has_errs ? " (with errors)" : "");
}

static void ktl_dump_footer(KTL_DumpAstContext *cont) {
    assert(cont);

    fprintf(cont->stream,
"<script>\n"
"document.addEventListener('click', function(e) {\n"
"  var t = e.target.closest('summary, tr, .leaf');\n"
"  if (!t) return;\n"
"  var el = t.closest('[data-sym-id]');\n"
"  if (!el) return;\n"
"  var id = el.getAttribute('data-sym-id');\n"
"  if (!id) return;\n"
"  document.querySelectorAll('.hl-sym').forEach(x=>x.classList.remove('hl-sym'));\n"
"  document.querySelectorAll('[data-sym-id=\"'+id+'\"]').forEach(function(x) {\n"
"    x.classList.add('hl-sym');\n"
"    var p = x.parentElement;\n"
"    while (p) {\n"
"      if (p.tagName === 'DETAILS' && !p.open) p.open = true;\n"
"      p = p.parentElement;\n"
"    }\n"
"  });\n"
"});\n"
"var cb = document.getElementById('show-types');\n"
"if (cb) cb.addEventListener('change', function() {\n"
"  document.body.classList.toggle('hide-annot', !cb.checked);\n"
"});\n"
"</script>\n"
"</body></html>\n");
}

// =======================================================================
// PRE-WALK: симсбол-реестр + статистика
// =======================================================================
static int ktl_sym_index(KTL_DumpAstContext *cont, KTL_SymbolEntry *e) {
    assert(cont);
    if (e == NULL)  return -1;

    for (int i = 0; i < cont->n_syms; i++) {
        if (cont->all_syms[i] == e)     return i;
    }
    return -1;
}

static void ktl_sym_register(KTL_DumpAstContext *cont, KTL_SymbolEntry *e) {
    assert(cont);
    if (e == NULL)  return ;
    if (ktl_sym_index(cont, e) >= 0)    return ;

    if (cont->n_syms >= cont->cap_syms) {
        int new_cap = cont->cap_syms * 2;
        KTL_SymbolEntry **buf = (KTL_SymbolEntry **)realloc(
            cont->all_syms, (size_t) new_cap * sizeof(KTL_SymbolEntry *));
        if (buf == NULL)    ExitF("NULL Realloc", );

        cont->all_syms = buf;
        cont->cap_syms = new_cap;
    }
    cont->all_syms[cont->n_syms++] = e;
}

static void ktl_register_map(KTL_DumpAstContext *cont, KTL_SymbolMap *map) {
    assert(cont);
    if (map == NULL)    return ;

    for (int i = 0; i < map->size; i++) {
        KTL_SymbolEntry *e = map->data[i];
        if (e == NULL)  continue;

        ktl_sym_register(cont, e);
        if (e->kind == KTL_SYMBOL_FUNC) {
            for (int j = 0; j < e->func.amount; j++) {
                ktl_sym_register(cont, e->func.params[j]);
            }
        }
    }
}

static void ktl_pre_walk(KTL_DumpAstContext *cont, KTL_AstNode *node) {
    assert(cont);
    if (node == NULL)   return ;

    cont->stats.total++;

    /* Регистрация локальных скоупов. */
    switch (node->kind) {
        case KTL_AST_BLOCK:
            ktl_register_map(cont, node->data.block.map);
            break;
        case KTL_AST_FOR_BLOCK:
            ktl_register_map(cont, node->data.for_block.map);
            break;
        case KTL_AST_FUNCTION_DECL:
            ktl_register_map(cont, node->data.func_decl.map);
            break;
        default:
            break;
    }

    /* Подсчёт raw vs resolved. */
    switch (node->kind) {
        case KTL_AST_VARIABLE:
            if (node->data.var.is_raw)              cont->stats.raw_vars++;
            else                                    cont->stats.resolved_vars++;
            break;
        case KTL_AST_FUNCTION_CALL:
            if (node->data.func_call.is_raw)        cont->stats.raw_calls++;
            else                                    cont->stats.resolved_calls++;
            break;
        default:
            break;
    }

    /* Подсчёт типизированных выражений. */
    KTL_TypeID t;
    if (ktl_get_expr_type(node, &t)) {
        cont->stats.exprs_total++;
        if (TypeIDCheck(t))     cont->stats.exprs_typed++;
        else                    cont->stats.bad_types++;
    }

    /* Рекурсия. */
    switch (KTL_AstGetTypeChildren(node)) {
        case KTL_AST_N_CHILDREN:
            for (int i = 0; i < node->move.n.amount; i++) {
                ktl_pre_walk(cont, node->move.n.children[i]);
            }
            break;
        case KTL_AST_BINARY_CHILDREN:
            ktl_pre_walk(cont, node->move.binary.left);
            ktl_pre_walk(cont, node->move.binary.right);
            break;
        case KTL_AST_UNARY_CHILD:
            ktl_pre_walk(cont, node->move.unary.next);
            break;
        case KTL_AST_NO_CHILDREN:
        default:
            break;
    }
}

// =======================================================================
// PIPELINE STATUS
// =======================================================================
static void ktl_dump_pipeline(KTL_DumpAstContext *cont) {
    assert(cont);

    KTL_DumpStats *s     = &cont->stats;
    KTL_DumpStage  stage = (KTL_DumpStage) cont->stage;

    fprintf(cont->stream,
            "<details class=\"section pipeline\" open>\n"
            "  <summary>Pipeline status — %s</summary>\n"
            "<div class=\"stat-grid\">\n",
            ktl_stage_name(stage));

    fprintf(cont->stream,
            "<span class=\"label\">Total nodes</span><span>%d</span>\n",
            s->total);

    /* Variable refs */
    int  v_total = s->raw_vars + s->resolved_vars;
    fprintf(cont->stream,
            "<span class=\"label\">Variable refs</span><span>%d "
            "<span class=\"%s\">(%d resolved)</span> "
            "<span class=\"%s\">(%d raw)</span></span>\n",
            v_total,
            s->resolved_vars > 0 ? "ok" : "label", s->resolved_vars,
            s->raw_vars      > 0 ? "warn" : "label", s->raw_vars);

    /* Function calls */
    int  c_total = s->raw_calls + s->resolved_calls;
    fprintf(cont->stream,
            "<span class=\"label\">Function calls</span><span>%d "
            "<span class=\"%s\">(%d resolved)</span> "
            "<span class=\"%s\">(%d raw)</span></span>\n",
            c_total,
            s->resolved_calls > 0 ? "ok" : "label", s->resolved_calls,
            s->raw_calls      > 0 ? "warn" : "label", s->raw_calls);

    /* Expressions */
    fprintf(cont->stream,
            "<span class=\"label\">Expressions</span><span>%d "
            "<span class=\"%s\">(%d typed)</span> "
            "<span class=\"%s\">(%d untyped)</span></span>\n",
            s->exprs_total,
            s->exprs_typed > 0 ? "ok" : "label", s->exprs_typed,
            s->bad_types   > 0
                ? (stage == KTL_STAGE_ANALYZED ? "bad" : "warn")
                : "label",
            s->bad_types);

    /* Подсказка по стадии */
    const char *hint = "";
    switch (stage) {
        case KTL_STAGE_PARSE:
            hint = "names not yet resolved — run name resolution";
            break;
        case KTL_STAGE_RESOLVED:
            hint = "types not yet inferred — run type analysis";
            break;
        case KTL_STAGE_ANALYZED:
            hint = (s->bad_types > 0)
                 ? "type analysis completed with errors"
                 : "type analysis completed";
            break;
        case KTL_STAGE_EMPTY:
            hint = "empty AST";
            break;
    }
    fprintf(cont->stream,
            "<span class=\"label\">Hint</span><span class=\"label\">%s</span>\n",
            hint);

    fprintf(cont->stream, "</div>\n</details>\n");
}

// =======================================================================
// GLOBAL SYM TABLE / TYPE TABLE
// =======================================================================
static void ktl_dump_global_syms(KTL_DumpAstContext *cont) {
    assert(cont);

    int sz = cont->global_map ? cont->global_map->size : 0;
    fprintf(cont->stream,
            "<details class=\"section\" open>\n"
            "  <summary>Global symbols (%d)</summary>\n", sz);

    if (sz == 0) {
        fprintf(cont->stream, "  <div class=\"empty\">empty</div>\n");
    } else {
        ktl_dump_sym_table(cont, cont->global_map);
    }

    fprintf(cont->stream, "</details>\n");
}

static void ktl_dump_types_table(KTL_DumpAstContext *cont) {
    assert(cont);

    KTL_TypeMap *map = cont->type_map;
    fprintf(cont->stream,
            "<details class=\"section\">\n"
            "  <summary>Types (%d)</summary>\n", map->size);

    if (map->size == 0) {
        fprintf(cont->stream, "  <div class=\"empty\">empty</div>\n");
    } else {
        fprintf(cont->stream,
"<table class=\"sym-table\">\n"
"<thead><tr><th>#</th><th>kind</th><th>form</th>"
"<th>size</th><th>align</th><th>extra</th></tr></thead>\n<tbody>\n");

        for (int i = 0; i < map->size; i++) {
            KTL_TypeEntry *e = &map->data[i];
            fprintf(cont->stream, "<tr><td class=\"idx\">%d</td>", i);

            switch (e->kind) {
                case KTL_TYPE_BASE:
                    fprintf(cont->stream,
                        "<td>base</td><td class=\"type\">%s</td>"
                        "<td>%d</td><td>%d</td><td></td>",
                        str_or(e->dt.base.name, "?"),
                        e->dt.base.size, e->dt.base.align);
                    break;
                case KTL_TYPE_BLOCK: {
                    fprintf(cont->stream,
                        "<td>block</td><td class=\"type\">%s</td>"
                        "<td>%d</td><td>%d</td><td>",
                        str_or(e->dt.block.name, "?"),
                        e->dt.block.size, e->dt.block.align);
                    fprintf(cont->stream, "%d field%s",
                            e->dt.block.field_count,
                            e->dt.block.field_count == 1 ? "" : "s");
                    if (!e->dt.block.complete) {
                        fprintf(cont->stream, " (incomplete)");
                    }
                    if (e->dt.block.field_count > 0) {
                        fprintf(cont->stream, ": ");
                        for (int j = 0; j < e->dt.block.field_count; j++) {
                            if (j > 0)  fprintf(cont->stream, ", ");
                            KTL_TypeField *f = &e->dt.block.fields[j];
                            fprintf(cont->stream, "%s: ",
                                    str_or(f->name, "?"));
                            ktl_type_print(cont->stream, map, f->base_type);
                        }
                    }
                    fprintf(cont->stream, "</td>");
                    break;
                }
                case KTL_TYPE_PTR:
                    fprintf(cont->stream,
                        "<td>ptr</td><td class=\"type\">");
                    ktl_type_print(cont->stream, map, i);
                    fprintf(cont->stream,
                        "</td><td>—</td><td>—</td><td>-&gt; #%d</td>",
                        e->dt.ptr.prev_type);
                    break;
                case KTL_TYPE_ARRAY:
                    fprintf(cont->stream,
                        "<td>array</td><td class=\"type\">");
                    ktl_type_print(cont->stream, map, i);
                    fprintf(cont->stream,
                        "</td><td>—</td><td>—</td>"
                        "<td>-&gt; #%d &times; %d</td>",
                        e->dt.arr.base_type, e->dt.arr.elem_count);
                    break;
                default:
                    fprintf(cont->stream, "<td>?</td><td></td>"
                                          "<td></td><td></td><td></td>");
                    break;
            }
            fprintf(cont->stream, "</tr>\n");
        }
        fprintf(cont->stream, "</tbody></table>\n");
    }

    if (map->alias_size > 0) {
        fprintf(cont->stream,
                "<h4 class=\"subhead\">Aliases (%d)</h4>\n"
                "<table class=\"sym-table\">\n"
                "<thead><tr><th>name</th><th>target</th></tr></thead>\n"
                "<tbody>\n",
                map->alias_size);
        for (int i = 0; i < map->alias_size; i++) {
            fprintf(cont->stream,
                "<tr><td class=\"name\">%s</td>"
                "<td class=\"type\">",
                str_or(map->aliases[i].name, "?"));
            ktl_type_print(cont->stream, map, map->aliases[i].target);
            fprintf(cont->stream, " <span class=\"kind\">(#%d)</span></td></tr>\n",
                    map->aliases[i].target);
        }
        fprintf(cont->stream, "</tbody></table>\n");
    }

    fprintf(cont->stream, "</details>\n");
}

static void ktl_dump_sym_table(KTL_DumpAstContext *cont, KTL_SymbolMap *map) {
    assert(cont);
    assert(map);

    fprintf(cont->stream,
"<table class=\"sym-table\">\n"
"<thead><tr><th>#</th><th>name</th><th>kind</th>"
"<th>type / signature</th><th>modifiers</th></tr></thead>\n<tbody>\n");

    for (int i = 0; i < map->size; i++) {
        KTL_SymbolEntry *e = map->data[i];
        if (e == NULL)  continue;

        int idx = ktl_sym_index(cont, e);
        fprintf(cont->stream,
            "<tr data-sym-id=\"%d\">"
            "<td class=\"idx\">%d</td>"
            "<td class=\"name\">%s</td>",
            idx, idx, str_or(e->str_id, "?"));

        if (e->kind == KTL_SYMBOL_VAR) {
            fprintf(cont->stream, "<td>var</td><td class=\"type\">");
            ktl_type_print(cont->stream, cont->type_map, e->var.type);
            fprintf(cont->stream, "</td><td>");
            ktl_dump_mods(cont->stream, e->var.mod);
            fprintf(cont->stream, "</td>");
        } else {
            fprintf(cont->stream, "<td>func</td><td class=\"type\">(");
            for (int j = 0; j < e->func.amount; j++) {
                if (j > 0)  fprintf(cont->stream, ", ");
                ktl_type_print(cont->stream, cont->type_map,
                               e->func.params[j]->var.type);
            }
            fprintf(cont->stream, ") -&gt; ");
            ktl_type_print(cont->stream, cont->type_map, e->func.ret_type);
            fprintf(cont->stream, "</td><td></td>");
        }
        fprintf(cont->stream, "</tr>\n");
    }
    fprintf(cont->stream, "</tbody></table>\n");
}

static void ktl_dump_scope_syms(KTL_DumpAstContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    KTL_SymbolMap *map = NULL;
    switch (node->kind) {
        case KTL_AST_BLOCK:         map = node->data.block.map;      break;
        case KTL_AST_FOR_BLOCK:     map = node->data.for_block.map;  break;
        case KTL_AST_FUNCTION_DECL: map = node->data.func_decl.map;  break;
        default:                                                     return;
    }
    if (map == NULL || map->size == 0)   return ;

    fprintf(cont->stream,
            "<details class=\"scope-syms\">\n"
            "  <summary>Scope symbols (%d)</summary>\n", map->size);
    ktl_dump_sym_table(cont, map);
    fprintf(cont->stream, "</details>\n");
}

static void ktl_dump_mods(FILE *s, int mod) {
    assert(s);
    bool first = true;

    #define EMIT(flag, name)                                                   \
        do {                                                                   \
            if (mod & (flag)) {                                                \
                if (!first)  fputs(", ", s);                                   \
                fputs(name, s);                                                \
                first = false;                                                 \
            }                                                                  \
        } while (0)

    EMIT(KTL_VAR_CONST,    "const");
    EMIT(KTL_VAR_MUTABLE,  "mutable");
    EMIT(KTL_VAR_INITIAL,  "init");
    EMIT(KTL_VAR_STACK,    "stack");
    EMIT(KTL_VAR_REGISTER, "register");

    #undef EMIT
}

// =======================================================================
// CORE TRAVERSAL
// =======================================================================
static void ktl_dump_node(KTL_DumpAstContext *cont,
                          KTL_AstNode        *node,
                          int                 depth) {
    assert(cont);
    if (node == NULL)   return;

    KTL_AstChildren  ch_kind = KTL_AstGetTypeChildren(node);
    bool             is_leaf = (ch_kind == KTL_AST_NO_CHILDREN);
    const char      *kname   = ktl_kind_name(node->kind);

    if (is_leaf) {
        fprintf(cont->stream,
                "<div class=\"node leaf node-%s\" data-kind=\"%s\" "
                "data-pos-line=\"%d\" data-pos-col=\"%d\"",
                kname, kname, node->pos.line, node->pos.column);
        ktl_dump_extra_attrs(cont, node);
        fprintf(cont->stream, ">");
        ktl_dump_summary   (cont, node);
        ktl_dump_type_annot(cont, node);
        ktl_dump_pos       (cont, node->pos);
        fprintf(cont->stream, "</div>\n");
        return ;
    }

    const char *open = (depth < KTL_DUMP_AUTO_OPEN_DEPTH) ? " open" : "";
    fprintf(cont->stream,
            "<details class=\"node node-%s\" data-kind=\"%s\" "
            "data-pos-line=\"%d\" data-pos-col=\"%d\"",
            kname, kname, node->pos.line, node->pos.column);
    ktl_dump_extra_attrs(cont, node);
    fprintf(cont->stream, "%s>\n  <summary>", open);
    ktl_dump_summary   (cont, node);
    ktl_dump_type_annot(cont, node);
    ktl_dump_pos       (cont, node->pos);
    fprintf(cont->stream, "</summary>\n");

    ktl_dump_scope_syms(cont, node);
    ktl_dump_children  (cont, node, depth + 1);

    fprintf(cont->stream, "</details>\n");
}

static void ktl_dump_children(KTL_DumpAstContext *cont,
                              KTL_AstNode        *node,
                              int                 depth) {
    assert(cont);
    assert(node);

    switch (KTL_AstGetTypeChildren(node)) {
        case KTL_AST_N_CHILDREN:
            for (int i = 0; i < node->move.n.amount; i++) {
                ktl_dump_node(cont, node->move.n.children[i], depth);
            }
            break;

        case KTL_AST_BINARY_CHILDREN:
            ktl_dump_node(cont, node->move.binary.left,  depth);
            ktl_dump_node(cont, node->move.binary.right, depth);
            break;

        case KTL_AST_UNARY_CHILD:
            ktl_dump_node(cont, node->move.unary.next, depth);
            break;

        case KTL_AST_NO_CHILDREN:
        default:
            break;
    }
}

// =======================================================================
// EXTRA HTML ATTRS
// =======================================================================
static void ktl_dump_extra_attrs(KTL_DumpAstContext *cont, KTL_AstNode *node) {
    assert(cont); assert(node);

    /* data-sym-id для подсветки */
    KTL_SymbolEntry *e = NULL;
    switch (node->kind) {
        case KTL_AST_VARIABLE:
            if (!node->data.var.is_raw)         e = node->data.var.info.res.entry;
            break;
        case KTL_AST_FUNCTION_CALL:
            if (!node->data.func_call.is_raw)   e = node->data.func_call.info.res.entry;
            break;
        case KTL_AST_VARIABLE_DECL:             e = node->data.var_decl.entry;  break;
        case KTL_AST_FUNCTION_DECL:             e = node->data.func_decl.func;  break;
        default:                                                                break;
    }
    int idx = ktl_sym_index(cont, e);
    if (idx >= 0) {
        fprintf(cont->stream, " data-sym-id=\"%d\"", idx);
    }

    /* data-typed: ok / bad / raw — для будущих фильтров и подсветки */
    KTL_DumpStage stage = (KTL_DumpStage) cont->stage;
    KTL_TypeID    t;

    bool is_raw = (node->kind == KTL_AST_VARIABLE      && node->data.var.is_raw)
               || (node->kind == KTL_AST_FUNCTION_CALL && node->data.func_call.is_raw);

    if (is_raw) {
        fprintf(cont->stream, " data-typed=\"raw\"");
    } else if (ktl_get_expr_type(node, &t)) {
        if      (TypeIDCheck(t))                   fprintf(cont->stream, " data-typed=\"ok\"");
        else if (stage == KTL_STAGE_ANALYZED)      fprintf(cont->stream, " data-typed=\"bad\"");
    }
}

// =======================================================================
// TYPE ANNOTATION (после summary, перед позицией)
// =======================================================================
static void ktl_dump_type_annot(KTL_DumpAstContext *cont, KTL_AstNode *node) {
    assert(cont); assert(node);

    if (!ktl_should_show_annot(node))   return ;

    KTL_TypeID t;
    if (!ktl_get_expr_type(node, &t))   return ;

    KTL_DumpStage stage = (KTL_DumpStage) cont->stage;

    if (TypeIDCheck(t)) {
        fprintf(cont->stream,
                "<span class=\"annot\">: <span class=\"type\">");
        ktl_type_print(cont->stream, cont->type_map, t);
        fprintf(cont->stream, "</span></span>");
    } else if (stage == KTL_STAGE_ANALYZED) {
        /* После анализа ожидаем валидный тип — отсутствие = ошибка. */
        fprintf(cont->stream,
                "<span class=\"annot bad\">: &#9888; bad type</span>");
    }
    /* На стадиях Parse/Resolved отсутствие типа — норма, ничего не печатаем. */
}

// =======================================================================
// SUMMARY
// =======================================================================
static void ktl_dump_summary(KTL_DumpAstContext *cont, KTL_AstNode *node) {
    assert(cont);
    assert(node);

    FILE *s = cont->stream;
    fprintf(s, "<span class=\"kind\">[%s]</span> ", ktl_kind_name(node->kind));

    switch (node->kind) {
        case KTL_AST_FILE:    fprintf(s, "FILE"); break;
        case KTL_AST_MAIN:    fprintf(s, "MAIN"); break;

        case KTL_AST_FUNCTION_DECL: {
            KTL_SymbolEntry *fn = node->data.func_decl.func;
            if (fn == NULL) { fprintf(s, "?"); break; }

            fprintf(s, "<span class=\"name\">%s</span>(",
                    str_or(fn->str_id, "?"));
            for (int i = 0; i < fn->func.amount; i++) {
                if (i > 0)  fprintf(s, ", ");
                fprintf(s, "<span class=\"type\">");
                ktl_type_print(s, cont->type_map,
                               fn->func.params[i]->var.type);
                fprintf(s, "</span> %s",
                        str_or(fn->func.params[i]->str_id, "?"));
            }
            fprintf(s, ") -&gt; <span class=\"type\">");
            ktl_type_print(s, cont->type_map, fn->func.ret_type);
            fprintf(s, "</span>");
            break;
        }

        case KTL_AST_FUNCTION_CALL: {
            const char *name;
            if (node->data.func_call.is_raw) {
                name = str_or(node->data.func_call.info.raw.name, "?");
            } else {
                KTL_SymbolEntry *e = node->data.func_call.info.res.entry;
                name = (e != NULL) ? str_or(e->str_id, "?") : "?";
            }
            fprintf(s, "<span class=\"name\">%s</span>"
                       "(<span class=\"kind\">%d args</span>)",
                       name, node->move.n.amount);
            if (node->data.func_call.is_raw) {
                fprintf(s, "<span class=\"raw\">raw</span>");
            }
            break;
        }

        case KTL_AST_VARIABLE: {
            const char *name;
            if (node->data.var.is_raw) {
                name = str_or(node->data.var.info.raw.name, "?");
            } else {
                KTL_SymbolEntry *e = node->data.var.info.res.entry;
                name = (e != NULL) ? str_or(e->str_id, "?") : "?";
            }
            fprintf(s, "<span class=\"name\">%s</span>", name);
            if (node->data.var.is_raw) {
                fprintf(s, "<span class=\"raw\">raw</span>");
            }
            break;
        }

        case KTL_AST_VARIABLE_DECL: {
            KTL_SymbolEntry *e = node->data.var_decl.entry;
            if (e == NULL) { fprintf(s, "?"); break; }
            fprintf(s, "<span class=\"type\">");
            ktl_type_print(s, cont->type_map, e->var.type);
            fprintf(s, "</span> <span class=\"name\">%s</span>%s",
                    str_or(e->str_id, "?"),
                    node->data.var_decl.is_init ? " = ..." : "");
            break;
        }

        case KTL_AST_ARRAY_INIT:
            fprintf(s, "array init <span class=\"kind\">(%d elems)</span>",
                    node->move.n.amount);
            break;

        case KTL_AST_FIELD_ACCESS:
            fprintf(s, "<span class=\"op\">%s</span>"
                       "<span class=\"name\">%s</span>",
                    node->data.field.is_ptr ? "-&gt;" : ".",
                    str_or(node->data.field.name, "?"));
            break;

        case KTL_AST_INDEX_ACCESS:
            fprintf(s, "<span class=\"op\">[ ]</span>");
            break;

        case KTL_AST_BINARY_OPER:
        case KTL_AST_UNARY_OPER:
            fprintf(s, "<span class=\"op\">%s</span>",
                    ktl_oper_symbol(node->data.oper.op));
            break;

        case KTL_AST_VALUE_INT:
            fprintf(s, "<span class=\"lit\">%lld</span>",
                    (long long)node->data.int_val.value);
            break;

        case KTL_AST_VALUE_STR:
            fprintf(s, "<span class=\"str\">\"");
            ktl_html_escape(s, str_or(node->data.str_val.value, ""));
            fprintf(s, "\"</span>");
            break;

        case KTL_AST_BLOCK:
            fprintf(s, "BLOCK <span class=\"kind\">(%d stmts%s)</span>",
                    node->move.n.amount,
                    node->data.block.map ? ", scope" : "");
            break;

        case KTL_AST_COND_BLOCK: fprintf(s, "cond");     break;
        case KTL_AST_IF_BRANCH:  fprintf(s, "if");       break;
        case KTL_AST_ELSE_BRANCH:fprintf(s, "else");     break;
        case KTL_AST_WHILE_BLOCK:fprintf(s, "while");    break;
        case KTL_AST_FOR_BLOCK:  fprintf(s, "for");      break;

        case KTL_AST_TYPEDEF:
            fprintf(s, "typedef <span class=\"name\">%s</span> = "
                       "<span class=\"type\">",
                    str_or(node->data.typedef_.alias, "?"));
            ktl_type_print(s, cont->type_map, node->data.typedef_.base_id);
            fprintf(s, "</span>");
            break;

        case KTL_AST_STRUCT_DECL:
            fprintf(s, "struct <span class=\"type\">");
            ktl_type_print(s, cont->type_map, node->data.struct_decl.type_id);
            fprintf(s, "</span>");
            break;

        case KTL_AST_ASSIGN:    fprintf(s, "<span class=\"op\">=</span>"); break;
        case KTL_AST_RETURN:    fprintf(s, "return");    break;
        case KTL_AST_BREAK:     fprintf(s, "break");     break;
        case KTL_AST_CONTINUE:  fprintf(s, "continue");  break;
        case KTL_AST_EXIT:      fprintf(s, "exit");      break;

        case KTL_AST_CAST:
            fprintf(s, "cast -&gt; <span class=\"type\">");
            ktl_type_print(s, cont->type_map, node->data.cast.target);
            fprintf(s, "</span>");
            break;

        default:
            fprintf(s, "?");
            break;
    }
}

// =======================================================================
// SMALL HELPERS
// =======================================================================
static void ktl_dump_pos(KTL_DumpAstContext *cont, KTL_SourcePos pos) {
    assert(cont);
    fprintf(cont->stream, "<span class=\"pos\">%d:%d</span>",
            pos.line, pos.column);
}

static void ktl_html_escape(FILE *s, const char *str) {
    if (str == NULL)    return ;

    for (; *str; str++) {
        switch (*str) {
            case '<':  fputs("&lt;",   s); break;
            case '>':  fputs("&gt;",   s); break;
            case '&':  fputs("&amp;",  s); break;
            case '"':  fputs("&quot;", s); break;
            case '\n': fputs("\\n",    s); break;
            case '\t': fputs("\\t",    s); break;
            default:   fputc(*str, s);     break;
        }
    }
}

static const char *ktl_kind_name(KTL_AstNodeKind kind) {
    switch (kind) {
        case KTL_AST_FILE:           return "FILE";
        case KTL_AST_MAIN:           return "MAIN";
        case KTL_AST_FUNCTION_DECL:  return "FUNCTION_DECL";
        case KTL_AST_FUNCTION_CALL:  return "FUNCTION_CALL";
        case KTL_AST_VARIABLE:       return "VARIABLE";
        case KTL_AST_VARIABLE_DECL:  return "VARIABLE_DECL";
        case KTL_AST_ARRAY_INIT:     return "ARRAY_INIT";
        case KTL_AST_FIELD_ACCESS:   return "FIELD_ACCESS";
        case KTL_AST_INDEX_ACCESS:   return "INDEX_ACCESS";
        case KTL_AST_BINARY_OPER:    return "BINARY_OPER";
        case KTL_AST_UNARY_OPER:     return "UNARY_OPER";
        case KTL_AST_VALUE_INT:      return "VALUE_INT";
        case KTL_AST_VALUE_STR:      return "VALUE_STR";
        case KTL_AST_BLOCK:          return "BLOCK";
        case KTL_AST_COND_BLOCK:     return "COND_BLOCK";
        case KTL_AST_IF_BRANCH:      return "IF_BRANCH";
        case KTL_AST_ELSE_BRANCH:    return "ELSE_BRANCH";
        case KTL_AST_WHILE_BLOCK:    return "WHILE_BLOCK";
        case KTL_AST_FOR_BLOCK:      return "FOR_BLOCK";
        case KTL_AST_TYPEDEF:        return "TYPEDEF";
        case KTL_AST_STRUCT_DECL:    return "STRUCT_DECL";
        case KTL_AST_ASSIGN:         return "ASSIGN";
        case KTL_AST_RETURN:         return "RETURN";
        case KTL_AST_BREAK:          return "BREAK";
        case KTL_AST_CONTINUE:       return "CONTINUE";
        case KTL_AST_EXIT:           return "EXIT";
        case KTL_AST_CAST:           return "CAST";
        default:                     return "?";
    }
}

static const char *ktl_oper_symbol(KTL_Oper op) {
    switch (op) {
        case KTL_OPER_ADD:        return "+";
        case KTL_OPER_SUB:        return "-";
        case KTL_OPER_MUL:        return "*";
        case KTL_OPER_DIV:        return "/";
        case KTL_OPER_MOD:        return "%";
        case KTL_OPER_NEG:        return "!";
        case KTL_OPER_AND:        return "&amp;&amp;";
        case KTL_OPER_OR:         return "||";
        case KTL_OPER_COMP_BE:    return "&gt;=";
        case KTL_OPER_COMP_B:     return "&gt;";
        case KTL_OPER_COMP_LE:    return "&lt;=";
        case KTL_OPER_COMP_L:     return "&lt;";
        case KTL_OPER_COMP_E:     return "==";
        case KTL_OPER_COMP_NE:    return "!=";
        case KTL_OPER_GET_PTR:    return "&amp;";
        case KTL_OPER_UNGET_PTR:  return "*";
        case KTL_OPER_ASSIGN:     return "=";
        default:                  return "?";
    }
}

// =======================================================================
// TYPE PRINTER
// =======================================================================
static void ktl_type_print(FILE *stream, KTL_TypeMap *map, KTL_TypeID id) {
    if (TypeIDCheck(map, id) == false) {
        fputs("?type?", stream);
        return ;
    }

    KTL_TypeEntry *entry = KTL_TypeGetEntry(map, id);
    if (entry == NULL) {
        fputs("?type?", stream);
        return ;
    }

    switch (entry->kind) {
        case KTL_TYPE_BASE:
            fputs(str_or(entry->dt.base.name,  "?"), stream);
            break;
        case KTL_TYPE_BLOCK:
            fputs(str_or(entry->dt.block.name, "?"), stream);
            break;
        case KTL_TYPE_PTR:
            ktl_type_print(stream, map, entry->dt.ptr.prev_type);
            fputc('*', stream);
            break;
        case KTL_TYPE_ARRAY:
            ktl_type_print(stream, map, entry->dt.arr.base_type);
            fprintf(stream, "[%d]", entry->dt.arr.elem_count);
            break;
        default:
            fputs("?type?", stream);
            break;
    }
}
