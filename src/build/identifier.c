
#include "../all.h"

Identifier *init_id() {
    Identifier *id = malloc(sizeof(Identifier));
    id->package = NULL;
    id->namespace = NULL;
    id->name = NULL;
    return id;
}

void free_id(Identifier *id) {
    //
    free(id);
}

IdentifierFor *init_idf() {
    IdentifierFor *idf = malloc(sizeof(IdentifierFor));
    idf->type = idfor_unknown;
    idf->item = NULL;
    return idf;
}

void free_idf(IdentifierFor *idf) {
    //
    free(idf);
}

char *create_c_identifier_with_strings(char *package, char *namespace, char *name) {
    char *result = malloc(KI_TOKEN_MAX);
    strcpy(result, "");

    if (strcmp(package, "main") != 0) {
        strcat(result, package);
        strcat(result, "__");
    }
    if (strcmp(namespace, "main") != 0) {
        strcat(result, namespace);
        strcat(result, "__");
    }
    strcat(result, name);

    return result;
}

char *fc_create_identifier_global_cname(FileCompiler *fc, Identifier *id) {
    PkgCompiler *pkc = fc->nsc->pkc;
    if (id->package != NULL) {
        pkc = pkc_get_by_name(id->package);
    }
    NsCompiler *nsc = fc->nsc;
    if (id->namespace != NULL) {
        nsc = pkc_get_namespace_by_name(pkc, id->namespace);
    }
    return create_c_identifier_with_strings(pkc->name, nsc->name, id->name);
}

Identifier *create_identifier(char *package, char *namespace, char *name) {
    Identifier *id = init_id();
    id->package = strdup(package);
    id->namespace = strdup(namespace);
    id->name = strdup(name);
    return id;
}

IdentifierFor *idf_find_in_scope(Scope *scope, Identifier *id) {
    char *vn = id->name;
    if (g_verbose_all) {
        printf("Find: %s\n", vn);
    }
    while (scope != NULL) {
        if (g_verbose_all) {
            printf("Scope: %p\n", scope);
        }
        void *x = map_get(scope->identifiers, vn);
        if (x != NULL) {
            return x;
        }
        scope = scope->parent;
    }
    return NULL;
}

Identifier *fc_read_identifier(FileCompiler *fc, bool readonly, bool sameline, bool allow_space) {
    //
    char *token = malloc(KI_TOKEN_MAX);
    Identifier *id = init_id();
    int i = fc->i;

    fc_next_token(fc, token, false, sameline, allow_space);
    if (!is_valid_varname(token)) {
        fc_error(fc, "Invalid name (id1): '%s'", token);
    }

    id->name = strdup(token);

    char ch = fc_get_char(fc, 0);
    if (ch == ':') {
        fc->i++;

        id->namespace = id->name;
        id->name = NULL;

        fc_next_token(fc, token, false, true, false);
        if (!is_valid_varname(token)) {
            if (allow_new_namespaces) {
                return NULL;
            }
            fc_error(fc, "Invalid name (id2): '%s'", token);
        }

        id->name = strdup(token);

        //
        ch = fc_get_char(fc, 0);
        if (ch == ':') {
            fc->i++;

            id->package = id->namespace;
            id->namespace = id->name;
            id->name = NULL;

            fc_next_token(fc, token, false, true, false);
            if (!is_valid_varname(token)) {
                if (allow_new_namespaces) {
                    return NULL;
                }
                fc_error(fc, "Invalid name: '%s'", token);
            }

            id->name = strdup(token);
        }
    }

    if (id->namespace == NULL) {
        int i = array_find(internal_types, token, "chars");
        if (i > -1) {
            id->package = strdup("ki");
            id->namespace = strdup("type");
        } else if (strcmp(token, "print") == 0) {
            id->package = strdup("ki");
            id->namespace = strdup("io");
        } else if (strcmp(token, "println") == 0) {
            id->package = strdup("ki");
            id->namespace = strdup("io");
        }
    }

    if (id->package == NULL && id->namespace != NULL) {
        // Check if it's a 'use' namespace
        IdentifierFor *idf = map_get(fc->scope->identifiers, id->namespace);
        if (idf && idf->type == idfor_namespace) {
            NsCompiler *nsc = idf->item;
            id->package = strdup(nsc->pkc->name);
            id->namespace = strdup(nsc->name);
        }
    }

    // Check for new namespaces
    if (allow_new_namespaces) {
        PkgCompiler *pkc = fc->nsc->pkc;
        if (id->package != NULL) {
            pkc = pkc_get_by_name(id->package);
        }
        if (id->namespace != NULL) {
            pkc_get_namespace_or_create(pkc, id->namespace);
        }
    }

    //
    if (readonly) {
        fc->i = i;
    }

    free(token);
    // printf("p:%s\n", id->package);
    // printf("n:%s\n", id->namespace);
    // printf("nn:%s\n", id->name);

    return id;
}

Scope *fc_get_identifier_scope(FileCompiler *fc, Scope *scope, Identifier *id) {
    Scope *idf_scope = scope;

    PkgCompiler *pkc = fc->nsc->pkc;
    if (id->package != NULL) {
        pkc = pkc_get_by_name(id->package);
    }
    NsCompiler *nsc = fc->nsc;
    if (id->namespace != NULL) {
        nsc = pkc_get_namespace_by_name(pkc, id->namespace);
        idf_scope = nsc->scope;
    }

    return idf_scope;
}

IdentifierFor *fc_read_and_get_idf(FileCompiler *fc, Scope *scope, bool readonly, bool sameline, bool allow_space) {
    Identifier *id = fc_read_identifier(fc, readonly, sameline, allow_space);
    Scope *idf_scope = fc_get_identifier_scope(fc, scope, id);
    return idf_find_in_scope(idf_scope, id);
}
