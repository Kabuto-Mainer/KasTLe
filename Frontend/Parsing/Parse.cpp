#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ParseType.h"
#include "ParseConfig.h"
#include "TypeMap.h"
#include "Common.h"


inline static KTL_Token * get_t(KTL_ParseContext *cont) {
    return cont->tokens + cont->cur_token;
}

inline static void next_t(KTL_ParseContext *cont) {
    if (cont->cur_token < cont->cap_token) {
        cont->cur_token++;
    }
}

inline static bool is_t_this(KTL_ParseContext *cont, KTL_ParseTokenRef ref) {
    KTL_Token *token = get_t(cont);
    if (token->kind != ref.kind) return false;

    switch (ref.kind) {
        case KTL_TOKEN_PUNCT: return token->data.punct == ref.as.punct;
        case KTL_TOKEN_KEY:   return token->data.key   == ref.as.key;
        default:              return false;
    }
}

inline static void add_node_l(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.binary.left = child;
}

inline static void add_node_r(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.binary.right = child;
}

inline static void add_node_u(KTL_AstNode *parent, KTL_AstNode *child) {
    parent->move.unary.next = child;
}

static KTL_Error add_node_n(KTL_AstNode *parent, KTL_AstNode *child) {
    if (parent->move.n.children = NULL) {
        parent->move.n.children = (KTL_AstNode **)calloc(1, sizeof(KTL_AstNode *));
        if (parent->move.n.children == NULL) {
            ExitF("NULL Calloc", KTL_MEMORY_ERR);
        }
        parent->move.n.amount = 1;
        parent->move.n.children[0] = child;
        return KTL_OK;
    }

    int size = parent->move.n.amount + 1;
    KTL_AstNode **buf = (KTL_AstNode **)realloc(parent->move.n.children, size * sizeof(KTL_AstNode *));
    if (buf == NULL) {
        ExitF("NULL Calloc", KTL_MEMORY_ERR);
    }

    buf[parent->move.n.amount ++] = child;
    parent->move.n.children = buf;

    return KTL_OK;
}

inline static int get_pose(KTL_ParseContext *cont) {
    return cont->cur_token;
}

inline static void set_pose(KTL_ParseContext *cont, int pose) {
    cont->cur_token = pose;
}


KTL_Error KTL_ParseInit(KTL_ParseContext *cont, KTL_StrMap *str_map) {
    assert(cont);
    assert(str_map);


}

KTL_AstNode * ktl_parse_var(KTL_ParseContext *cont)
{
    if (get_t(cont)->kind != KTL_TOKEN_STRING) {
        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    node->kind = KTL_AST_VARIABLE;
    node->data.var.info.raw.name = get_t(cont)->data.string;

    next_t(cont);
    return node;
}

KTL_AstNode * ktl_parse_var_decl(KTL_ParseContext *cont, KTL_SymbolMap *cur_map) {
    assert(cont);
    assert(cur_map);

    if (is_t_this(cont, KTL_KEY_VAR_DECL) == false) {
        return NULL;
    }

    next_t(cont);
    KTL_TypeID type_var = ktl_parse_type(cont);
    if (TypeIDCheck(type_var) == false) {
        return NULL;
    }

    KTL_StrID name_var = ktl_parse_name(cont);
    if (StrIDCheck(name_var) == false) {
        return NULL;
    }

    bool is_init = false;
    KTL_AstNode *next = NULL;

    if (is_t_this(cont, KTL_KEY_ASSIGN)) {
        is_init = true;
        next_t(cont);

        next = ktl_parse_expr(cont);
    }

    KTL_AstNode *node = ktl_alloc_node();
    node->kind = KTL_AST_VARIABLE_DECL;
    node->data.var_decl.entry = KTL_SymbolInsert(cur_map, KTL_SYMBOL_VAR, type_var, name_var);
    node->data.var_decl.is_init = is_init;
    node->move.unary.next = next;

    return node;
}

KTL_AstNode * ktl_parse_func_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (is_t_this(cont, KTL_KEY_FUNC_DECL) == false) {
        return NULL;
    }
    next_t(cont);

    KTL_StrID name_func = ktl_parse_name(cont);
    if (StrIDCheck(name_func) == false) {
        return NULL;
    }

    KTL_TypeID type_func = ktl_parse_type(cont);
    if (TypeIDCheck(type_func) == false) {
        return NULL;
    }

    if (is_t_this(cont, KTL_PUNCT_LEFT_ROUND) == false) {
        return NULL;
    }
    next_t(cont);

    KTL_SymbolMap *func_map = KTL_SymbolMapInit();

    int counter = 0;
    while (true) {
        if (is_t_this(cont, KTL_PUNCT_RIGHT_ROUND) == true) {
            break;
        }

        KTL_TypeID type_param = ktl_parse_type(cont);
        if (KTL_TypeID(type_param) == false) {
            KTL_SymbolMapUninit(func_map);
            return NULL;
        }

        KTL_StrID name_param = ktl_parse_name(cont);
        if (StrIDCheck(name_param) == false) {
            KTL_SymbolMapUninit(func_map);
            return NULL;
        }

        KTL_SymbolInsert(cont, KTL_SYMBOL_VAR, type_param, name_param);
        if (is_t_this(cont, KTL_PUNCT_COMMA) == true) {
            next_t(cont);
            continue;
        }
        break;
    }
    if (is_t_this(cont, KTL_PUNCT_RIGHT_ROUND) == false) {
        KTL_SymbolMapUninit(func_map);
        return NULL;
    }
    next_t(cont);

    if (is_t_this(cont, KTL_PUNCT_LEFT_FIGURE) == false) {
        KTL_SymbolMapUninit(func_map);
        return NULL;
    }

    KTL_AstNode *node = ktl_alloc_node();
    node->kind = KTL_AST_FUNCTION_DECL;
    node->data.func_decl.map = func_map;
    node->data.func_decl.func = KTL_SymbolInsert(cont->func_map, KTL_SYMBOL_FUNC, type_func, name_func);

    while (true) {
        KTL_AstNode *child = ktl_parse_line(cont);
        if (child == NULL) {
            child = ktl_parse_return(cont);
        }
        if (child == NULL) {
            KTL_SymbolMapUninit(func_map);
            return NULL;
        }

        add_node_n(node, child);
    }

    if (is_t_this(cont, KTL_PUNCT_RIGHT_FIGURE) == false) {
        KTL_SymbolMapUninit(func_map);
        return NULL;
    }
    next_t(cont);

    return node;
}

KTL_AstNode * ktl_parse_line(KTL_ParseContext *cont, KTL_SymbolMap *cur_map) {
    assert(cont);
    assert(cur_map);

    KTL_AstNode *node = ktl_parse_var_decl(cont, cur_map);
    if (node == NULL) {
        node = ktl_parse_assign(cont);
    }
    if (node == NULL) {
        node = ktl_parse_call(cont);
    }
    if (node == NULL) {
        node = ktl_parse_condition(cont, cur_map);
    }
    if (node == NULL) {
        node = ktl_parse_for(cont, cur_map);
    }
    if (node == NULL) {
        node = ktl_parse_while(cont, cur_map);
    }

}

KTL_AstNode * ktl_parse_assign(KTL_ParseContext *cont) {
    assert(cont);

    int pose = get_pose(cont);
    KTL_AstNode *var = ktl_parse_var(cont);
    if (var == NULL || is_t_this(cont, KTL_KEY_ASSIGN) == false) {
        set_pose(cont, pose);
        return NULL;
    }
    next_t(cont);

    KTL_AstNode *value = ktl_parse_expr(cont);
    if (value == NULL) {
        ktl_destroy_node(var);
        return NULL;
    }


}

KTL_AstNode * ktl_parse_call(KTL_ParseContext *cont) {
    assert(cont);

    int pose = get_pose(cont);
}


KTL_AstNode * ktl_parse_type_decl(KTL_ParseContext *cont) {
    assert(cont);

    if (is_t_this(cont, KTL_KEY_TYPEDEF)) {
        return ktl_parse_typedef(cont);
    }
    if (is_t_this(cont, KTL_KEY_STRUCT)) {
        return ktl_parse_struct(cont);
    }
    return NULL;
}

KTL_Error ktl_parse_typedef(KTL_ParseContext *cont) {
    assert(cont);

    next_t(cont);
    KTL_TypeID base_type = ktl_parse_type(cont);
    KTL_StrID new_type = ktl_parse_name(cont);

    KTL_TypeAddDefine(cont->type_map, base_type, new_type);

    return KTL_OK;
}

KTL_Error ktl_parse_struct(KTL_ParseContext *cont) {
    assert(cont);

    next_t(cont);
    KTL_StrID struct_name = ktl_parse_name(cont);

    if (is_t_this(cont, KTL_PARSE_STRUCT_LEFT) == false) {
        return KTL_LOGICAL_ERR;
    }
    next_t(cont);

    KTL_TypeID struct_id = KTL_TypeAddBlock(cont->type_map, struct_name);
    int counter = 0;

    while (true) {
        KTL_TypeID type_field = ktl_parse_type(cont);
        if (TypeIDCheck(type_field) == false) {
            return KTL_LOGICAL_ERR;
        }

        KTL_StrID name_field = ktl_parse_name(cont);
        if (StrIDCheck(name_field) == false) {
            return KTL_LOGICAL_ERR;
        }

        KTL_TypeBlockAddField(cont->type_map, struct_id, type_field, name_field);
        counter++;

        if (is_t_this(cont, KTL_PARSE_END_LINE) == false) {
            return KTL_LOGICAL_ERR;
        }
        next_t(cont);
    }

    if (counter == 0) {
        return KTL_LOGICAL_ERR;
    }
    KTL_TypeBlockFinish(cont->type_map, struct_id);

    if (is_t_this(cont, KTL_PARSE_STRUCT_RIGHT) == false) {
        return KTL_LOGICAL_ERR;
    }
    next_t(cont);

    return KTL_OK;
}

KTL_TypeID ktl_parse_type(KTL_ParseContext *cont) {
    assert(cont);

    KTL_Token *token = get_t(cont);
    if (token->kind != KTL_TOKEN_STRING) {
        return KTL_BAD_TYPE_ID;
    }

    KTL_StrID name_type_id = token->data.string;
    KTL_TypeID type_id = KTL_TypeFindBase(cont->type_map, name_type_id);
    if (TypeIDCheck(type_id) == false) {
        return KTL_BAD_TYPE_ID;
    }

    next_t(cont);
    while (is_t_this(cont, KTL_PUNCT_MUL)) {
        type_id = KTL_TypeAddPointer(cont->type_map, type_id);
        next_t(cont);
    }

    while (is_t_this(cont, KTL_PUNCT_LEFT_TRIG)) {
        next_t(cont);
        token = get_t(cont);
        if (token->kind != KTL_TOKEN_VALUE || token->data.value <= 0) {
            return KTL_BAD_TYPE_ID;
        }
        int size = token->data.value;

        next_t(cont);
        if (is_t_this(cont, KTL_PUNCT_RIGHT_TRIG) == false) {
            return KTL_BAD_TYPE_ID;
        }

        type_id = KTL_TypeAddArray(cont->type_map, type_id, size);
        next_t(cont);
    }

    return type_id;
}

KTL_StrID ktl_parse_name(KTL_ParseContext *cont) {
    assert(cont);

    KTL_Token *token = get_t(cont);
    if (token->kind != KTL_TOKEN_STRING) {
        return KTL_BAD_STR_ID;
    }

    next_t(cont);
    return token->data.string;
}



KTL_AstNode * ktl_alloc_node() {
    KTL_AstNode *node = (KTL_AstNode *)calloc(1, sizeof(KTL_AstNode));
    if (node == NULL) {
        ExitF("NULL Calloc", NULL);
    }
    return node;
}
