
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
    char *token = fc->token;
    Id *id = fc->id_buf;
    id->has_nsc = false;

    tok(fc, token, sameline, allow_space);

    // if (token[0] == ':') {
    //     strcpy(token, "main");
    //     fc->i--;
    // }

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
        tok(fc, token, true, false);

        // if (token[0] == ':') {
        //     strcpy(token, "main");
        //     fc->i--;
        // }

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

Idf *idf_by_id(Fc *fc, Scope *scope, Id *id, bool fail) {
    //

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
        idf = map_get(scope->identifiers, name);
        //
        if (!idf) {
            scope = scope->parent;
            if (!scope) {

                if (!id->has_nsc) {
                    sprintf(fc->sbuf, ".%s.", name);
                    if (strstr(".bool.ptr.i8.u8.i16.u16.i32.u32.i64.u64.ixx.uxx.fxx.String.cstring.Array.Map.AsyncArray.AsyncMap.", fc->sbuf)) {
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
        die(b->sbuf);
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
