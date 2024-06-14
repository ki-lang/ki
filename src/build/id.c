
#include "../all.h"

Id *id_init(Allocator *alc) {
    Id *id = al(alc, sizeof(Id));
    id->nsc_name = al(alc, KI_TOKEN_MAX);
    id->name = al(alc, KI_TOKEN_MAX);
    id->has_nsc = false;
    return id;
}

Idf *idf_init(Allocator *alc, int type) {
    Idf *idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = NULL;
    return idf;
}
Idf *idf_init_item(Allocator *alc, int type, void *item) {
    Idf *idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}

Id *read_id(Fc *fc, bool sameline, bool allow_space, bool crash) {
    //
    Id *id = fc->id_buf;
    id->has_nsc = false;

    char* token = tok(fc, sameline, allow_space);

    if (!is_valid_varname(token)) {
        if (!crash)
            return NULL;
        sprintf(fc->sbuf, "Invalid identifier: '%s'", token);
        fc_error(fc);
    }

    if (get_char(fc, 0) == ':') {
        id->has_nsc = true;
        strcpy(id->nsc_name, token);
        chunk_move(fc->chunk, 1);
        token = tok(fc, true, false);

        if (!is_valid_varname(token)) {
            if (!crash)
                return NULL;
            sprintf(fc->sbuf, "Invalid identifier: '%s'", token);
            fc_error(fc);
        }
    }

    strcpy(id->name, token);

    return id;
}

Idf *read_idf(Fc *fc, Scope *scope, bool sameline, bool allow_space) {
    //
    bool lsp = fc->lsp_file && lsp_check(fc);
    Build *b = fc->b;

    char* token = tok(fc, sameline, allow_space);

    Idf *idf = NULL;

    if (!is_valid_varname(token)) {
        sprintf(fc->sbuf, "Invalid identifier: '%s'", token);
        fc_error(fc);
    }

    Id id;
    id.has_nsc = false;
    id.name = token;
    unsigned long start = microtime();
    idf = idf_by_id(fc, scope, &id, false);
    // b->time_parse += microtime() - start;

    if (idf && idf->type == idf_nsc && get_char(fc, 0) == ':') {

        chunk_move(fc->chunk, 1);

        Nsc *nsc = idf->item;

        lsp = lsp || (fc->lsp_file && lsp_check(fc));

        // LSP Completion
        if (lsp && b->lsp->type == lspt_completion) {
            LspData *ld = b->lsp;
            Chunk *chunk = fc->chunk;
            Allocator *alc = fc->alc;
            Array *items = array_make(alc, 100);
            Scope *cur_scope = nsc->scope;
            while (cur_scope) {
                Map *identifiers = cur_scope->identifiers;
                Array *names = identifiers->keys;
                Array *idfs = identifiers->values;
                for (int i = 0; i < names->length; i++) {
                    Idf *idf = array_get_index(idfs, i);
                    char *name = array_get_index(names, i);
                    int type = lsp_compl_property;
                    if (idf->type == idf_func) {
                        type = lsp_compl_function;
                    }
                    LspCompletion *c = lsp_completion_init(alc, type, name);
                    if (idf->type == idf_func) {
                        Func *func = idf->item;
                        if (nsc != fc->nsc && func->act == act_private) {
                            continue;
                        }
                        c->label = lsp_func_label(alc, func, name, true);
                        c->insert = lsp_func_insert(alc, func, name, false);
                    }
                    array_push(items, c);
                }
                cur_scope = cur_scope->parent;
            }
            lsp_completion_respond(b->alc, ld, items);
            build_end(b, 0);
        }

        token = tok(fc, true, false);

        id.has_nsc = false;
        id.name = token;
        idf = idf_by_id(fc, nsc->scope, &id, false);
    }

    if (idf && idf->type == idf_fc && get_char(fc, 0) == '.') {
        Fc *rfc = idf->item;
        if (rfc->is_header) {
            chunk_move(fc->chunk, 1);

            char buf[256];
            strcpy(buf, token);
            token = tok(fc, true, false);

            idf = idf_get_from_header(rfc, token, 0);
            if (!idf) {
                sprintf(fc->sbuf, "Identifier '%s' not found in '%s'", token, rfc->path_ki);
                fc_error(fc);
            }
        }
    }

    if (!idf) {
        sprintf(fc->sbuf, "Unknown identifier: '%s'", token);
        fc_error(fc);
    }

    if (lsp && fc->b->lsp->type == lspt_definition) {
        char *path = NULL;
        int line = 0;
        int col = 0;
        if (idf->type == idf_class) {
            Class *class = idf->item;
            path = class->fc->path_ki;
            line = class->def_chunk->line;
            col = class->def_chunk->col;
        } else if (idf->type == idf_func) {
            Func *func = idf->item;
            path = func->fc->path_ki;
            line = func->def_chunk->line;
            col = func->def_chunk->col;
        } else if (idf->type == idf_global) {
            Global *g = idf->item;
            path = g->fc->path_ki;
            line = g->def_chunk->line;
            col = g->def_chunk->col;
        } else if (idf->type == idf_trait) {
            Trait *t = idf->item;
            path = t->fc->path_ki;
            line = t->def_chunk->line;
            col = t->def_chunk->col;
        } else if (idf->type == idf_enum) {
            Enum *enu = idf->item;
            path = enu->fc->path_ki;
            line = enu->def_chunk->line;
            col = enu->def_chunk->col;
            // } else if (idf->type == idf_decl) {
            //     Decl *decl = idf->item;
            //     path = decl->fc->path_ki;
            //     line = decl->chunk_body->line;
            //     col = decl->chunk_body->col;
        }
        if (path) {
            lsp_definition_respond(b->alc, b->lsp, path, line - 1, col - 1);
            build_end(b, 0);
        }
    }

    return idf;
}

Idf *idf_by_id(Fc *fc, Scope *scope, Id *id, bool fail) {
    //
    Build* b = fc->b;

    if (id->has_nsc) {
        Scope *fc_scope = scope_find(scope, sct_fc);
        Idf *idf = map_get(fc_scope->identifiers, id->nsc_name);
        if (!idf || idf->type != idf_nsc) {
            if (!fail) {
                return NULL;
            }
            sprintf(fc->sbuf, "Unknown identifier (unknown/un-used namespace): '%s:%s'", id->nsc_name, id->name);
            fc_error(fc);
        }
        Nsc *nsc = idf->item;
        scope = nsc->scope;
    }

    Idf *idf = NULL;
    char *name = id->name;
    while (!idf) {
        //
    unsigned long start = microtime();
        idf = map_get(scope->identifiers, name);
                b->time_parse += microtime() - start;
        //
        if (!idf) {
            scope = scope->parent;
            if (!scope) {

                if (!id->has_nsc) {
                    sprintf(fc->sbuf, ".%s.", name);
                    if (strstr(".bool.ptr.i8.u8.i16.u16.i32.u32.i64.u64.ixx.uxx.fxx.String.c_string.array.Array.map.Map.AsyncArray.AsyncMap.ByteBuffer.ByteBufferStruct.", fc->sbuf)) {
                        return ki_lib_get(fc->b, "type", name);
                    }
                    if (strstr(".print.println.", fc->sbuf)) {
                        return ki_lib_get(fc->b, "io", name);
                    }
                    if (strstr(".exit.panic.", fc->sbuf)) {
                        return ki_lib_get(fc->b, "os", name);
                    }
                    // if (strstr(".Task.", fc->sbuf)) {
                    //     return ki_lib_get("async", name);
                    // }
                }

                if (!fail) {
                    return NULL;
                }

                sprintf(fc->sbuf, "Unknown identifier: '%s'", id->name);
                fc_error(fc);
            }
        }
    }

    return idf;
}

Idf *ki_lib_get(Build *b, char *ns, char *name) {
    //
    Nsc *nsc = pkc_get_nsc(b->pkc_ki, ns);
    Idf *idf = map_get(nsc->scope->identifiers, name);
    if (!idf) {
        sprintf(b->sbuf, "ki lib identifier not found: '%s'", name);
        build_error(b, b->sbuf);
    }
    return idf;
}

Idf *idf_get_from_header(Fc *hfc, char *name, int depth) {
    //
    if (depth > 5) {
        return NULL;
    }
    Idf *idf = map_get(hfc->scope->identifiers, name);
    if (idf) {
        return idf;
    }
    depth++;
    for (int i = 0; i < hfc->sub_headers->length; i++) {
        Fc *sfc = array_get_index(hfc->sub_headers, i);
        idf = idf_get_from_header(sfc, name, depth);
        if (idf) {
            return idf;
        }
    }
    return NULL;
}
