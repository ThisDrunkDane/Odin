void h1(String str) {
    switch(docs_context.doc_format) {
        case DocsFormat_Markdown :
        case DocsFormat_Slate : {
            DOC_PRINT("# %.*s\n", LIT(str));
            break;
        }

        case DocsFormat_Html : {
            DOC_PRINT("<h1>%.*s</h1>\n", LIT(str));
            break;
        }
    }
}

void h1(AstNodeIdent *id) {
    h1(id->token.string);
}

void h1(char *str) {
    h1(make_string(cast(u8*)str, strlen(str)));
}

void h2(String str) {
    switch(docs_context.doc_format) {
        case DocsFormat_Markdown :
        case DocsFormat_Slate : {
            DOC_PRINT("## %.*s\n", LIT(str));
            break;
        }

        case DocsFormat_Html : {
            DOC_PRINT("<h2>%.*s</h2>\n", LIT(str));
            break;
        }
    }
}

void h2(AstNodeIdent *id) {
    h2(id->token.string);
}

void h2(char *str) {
    h2(make_string(cast(u8*)str, strlen(str)));
}

void h3(String str) {
    switch(docs_context.doc_format) {
        case DocsFormat_Markdown :
        case DocsFormat_Slate : {
            DOC_PRINT("### %.*s\n", LIT(str));
            break;
        }

        case DocsFormat_Html : {
            DOC_PRINT("<h3>%.*s</h3>\n", LIT(str));
            break;
        }
    }
}

void h3(AstNodeIdent *id) {
    h3(id->token.string);
}

void h3(char *str) {
    h3(make_string(cast(u8*)str, strlen(str)));
}

void bold(char* fmt, ...) {
    char* fmt_str = "";
    switch(docs_context.doc_format) {
        case DocsFormat_Markdown :
        case DocsFormat_Slate : {
            fmt_str = "**%s**";
            break;
        }

        case DocsFormat_Html : {
            fmt_str = "<b>%s</b>";
            break;
        }
    }
    fmt_str = gb_bprintf(fmt_str, fmt);
    va_list va;
    va_start(va, fmt);
    gb_fprintf_va(&docs_context.doc_file, fmt_str, va);
    va_end(va);
}

void doc_location(AstNodeIdent *ident) {
    bold("Location: ");
    DOC_PRINT("%.*s:%d", LIT(ident->token.pos.file), ident->token.pos.line);
}