#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <llib/file.h>
#include <llib/json.h>
#include <llib/scan.h>
#include <llib/str.h>

#define LINE_SIZE 4096

static PValue directive_field(const char *type, const char *name,
                              const char *desc);
static const char *record_string(SMap m, const char *key);

typedef enum { OUT_MARKDOWN, OUT_JSON } OutputFormat;

typedef struct {
    char *name;
    PValue module;
    PValue **items;
    char ***sources;
} ModuleDoc;

typedef struct {
    char *name;
    char *arg;
    char *rest;
} Directive;

static void directive_dispose(Directive *d) {
    obj_unref(d->name);
    obj_unref(d->arg);
    obj_unref(d->rest);
}

static void module_doc_dispose(ModuleDoc *doc) {
    obj_unref(doc->name);
    obj_unref(doc->module);
    obj_unref(doc->items);
    obj_unref(doc->sources);
}

static const char *skip_space(const char *s) {
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

static void trim_right(char *s) {
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

static const char *directive_name(const char *directive) {
    return *directive == '@' ? directive + 1 : directive;
}

static Directive *directive_from_line(const char *line) {
    Directive *dir = obj_new(Directive, directive_dispose);
    ScanState *sc = scan_new_from_string(line);
    char buff[LINE_SIZE];

    dir->name = NULL;
    dir->arg = NULL;
    dir->rest = NULL;
    scan_set_flags(sc, C_IDEN);
    if (scan_next(sc) != '@')
        goto fail;
    if (scan_next(sc) != T_TOKEN)
        goto fail;
    scan_get_tok(sc, buff, sizeof(buff));
    dir->name = str_new(buff);
    if (scan_next(sc) == T_TOKEN) {
        scan_get_tok(sc, buff, sizeof(buff));
        dir->arg = str_new(buff);
    } else {
        scan_push_back(sc);
        dir->arg = str_new("");
    }
    scan_get_line(sc, buff, sizeof(buff));
    dir->rest = str_new(buff);
    obj_unref(sc);
    return dir;

fail:
    obj_unref(sc);
    obj_unref(dir);
    return NULL;
}

static char *directive_value_from_line(const char *line, const char *directive) {
    scoped Directive *dir = directive_from_line(line);

    if (!dir || !str_eq(dir->name, directive_name(directive)))
        return NULL;
    if (*dir->arg)
        return str_new(dir->arg);
    return str_new(dir->rest);
}

static char *find_directive_value(const char *doc, const char *directive) {
    const char *line = doc;

    while (*line) {
        const char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) : strlen(line);
        scoped char *copy = str_trim_range(line, len);
        char *value = directive_value_from_line(copy, directive);

        if (value)
            return value;
        if (!next)
            break;
        line = next + 1;
    }
    return NULL;
}

static char *doc_without_directives(const char *doc) {
    char **clean = strbuf_new();
    const char *line = doc;

    while (*line) {
        const char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) : strlen(line);
        scoped char *copy = str_trim_range(line, len);

        if (!directive_from_line(copy)) {
            strbuf_addr(clean, line, 0, len);
            if (next)
                strbuf_add(clean, '\n');
        }
        if (!next)
            break;
        line = next + 1;
    }
    return strbuf_tostring(clean);
}

static PValue directive_field_from_line(const char *line) {
    scoped Directive *dir = directive_from_line(line);

    if (!dir || str_eq(dir->name, "table") || !*dir->arg)
        return NULL;
    return directive_field(dir->name, dir->arg, dir->rest);
}

static char *extract_function_name(const char *sig, const char **args_start) {
    ScanState *sc = scan_new_from_string(sig);
    char *name = str_new("");
    char buff[LINE_SIZE];
    char *res = NULL;

    *args_start = NULL;
    scan_set_flags(sc, C_IDEN);
    while (scan_next(sc) != T_END) {
        if (sc->type == '(') {
            *args_start = sig + scan_position(sc);
            res = str_new(name);
            break;
        }
        if (sc->type == T_TOKEN) {
            scan_get_tok(sc, buff, sizeof(buff));
            obj_unref(name);
            name = str_new(buff);
        }
    }
    obj_unref(sc);
    obj_unref(name);
    return res ? res : str_new("");
}

static char *trim_signature(const char *sig, const char *args_start) {
    const char *end = args_start ? strstr(args_start, ")") : NULL;

    if (end)
        return str_trim_range(sig, (int)(end - sig + 1));
    return str_trim_range(sig, -1);
}

static bool has_directive(const char *doc, const char *directive) {
    scoped char *value = find_directive_value(doc, directive);
    return value != NULL;
}

static char *next_signature(FILE *f, char *line, size_t line_size) {
    while (fgets(line, line_size, f)) {
        char *p = (char *)skip_space(line);
        if (*p == '\0')
            continue;
        trim_right(p);
        return str_new(p);
    }
    return str_new("");
}

static void append_doc_line(char **doc, const char *p) {
    if (*p == ' ')
        p++;
    strbuf_adds(doc, p);
}

static void record_put_str(char ***m, const char *key, const char *value) {
    smap_put_ref(m, key, value ? value : "");
}

static void record_update_str(PValue *record, const char *key,
                              const char *value) {
    smap_put_ref((char ***)record, key, value ? value : "");
}

static PValue directive_field(const char *type, const char *name,
                              const char *desc) {
    char ***field = smap_new(true);

    record_put_str(field, "type", type);
    record_put_str(field, "name", name);
    record_put_str(field, "doc", desc);
    return smap_close(field);
}

static PValue table_fields(const char *doc) {
    PValue **fields = seq_new_ref(PValue);
    const char *line = doc;

    while (*line) {
        const char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) : strlen(line);
        scoped char *copy = str_trim_range(line, len);
        PValue field = directive_field_from_line(copy);

        if (field)
            seq_add(fields, field);
        if (!next)
            break;
        line = next + 1;
    }

    return seq_array_ref(fields);
}

static char *module_name_from_doc(const char *doc) {
    char *name = find_directive_value(doc, "@module");

    return name ? name : find_directive_value(doc, "@submodule");
}

static char *path_basename_no_ext(const char *path) {
    scoped char *base = file_basename(path);
    const char *dot = strrchr(base, '.');

    if (dot && dot > base)
        return str_trim_range(base, (int)(dot - base));
    return str_new(base);
}

static bool source_extension(const char *path) {
    scoped char *ext = file_extension(path);

    return str_eq_any(ext, ".c", ".h") != 0;
}

static bool mkdir_p(const char *path) {
    scoped char *copy = str_new(path);

    for (char *p = copy + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(copy, 0777) != 0 && errno != EEXIST) {
                perror(copy);
                return false;
            }
            *p = '/';
        }
    }
    if (mkdir(copy, 0777) != 0 && errno != EEXIST) {
        perror(copy);
        return false;
    }
    return true;
}

static bool skip_dir_name(const char *name) {
    return str_eq(name, ".") || str_eq(name, "..") || str_eq(name, ".git") ||
           str_eq(name, ".vscode") || str_eq(name, "build");
}

static bool collect_sources(const char *path, char ***sources) {
    struct stat st;

    if (stat(path, &st) != 0) {
        perror(path);
        return false;
    }
    if (S_ISREG(st.st_mode)) {
        if (source_extension(path))
            seq_add(sources, str_new(path));
        return true;
    }
    if (S_ISDIR(st.st_mode)) {
        scoped char **entries = file_dir_entries(path);

        if (!entries) {
            perror(path);
            return false;
        }
        FOR (i, array_len(entries)) {
            scoped char *child = NULL;
            const char *entry = entries[i];

            if (skip_dir_name(entry))
                continue;
            child = file_path_join(path, entry);
            if (!collect_sources(child, sources))
                return false;
        }
    }
    return true;
}

static int cmp_str_ptr(const void *a, const void *b) {
    const char *const *sa = a;
    const char *const *sb = b;
    const unsigned char *pa = (const unsigned char *)*sa;
    const unsigned char *pb = (const unsigned char *)*sb;

    while (*pa && *pb && *pa == *pb) {
        pa++;
        pb++;
    }
    if (*pa == *pb)
        return 0;
    if (*pa == '.' && *pb == '-')
        return -1;
    if (*pa == '-' && *pb == '.')
        return 1;
    return *pa < *pb ? -1 : 1;
}

static char *safe_markdown_name(const char *name) {
    char **out = strbuf_new();
    bool last_dash = false;

    for (const char *p = name; *p; p++) {
        unsigned char ch = (unsigned char)*p;

        if (isalnum(ch) || ch == '_' || ch == '-') {
            strbuf_add(out, (char)ch);
            last_dash = false;
        } else if (!last_dash) {
            strbuf_add(out, '-');
            last_dash = true;
        }
    }
    if (strlen(*out) == 0)
        strbuf_adds(out, "module");
    strbuf_adds(out, ".md");
    return strbuf_tostring(out);
}

static ModuleDoc *module_doc_new(const char *name) {
    ModuleDoc *doc = obj_new(ModuleDoc, module_doc_dispose);

    doc->name = str_new(name);
    doc->module = NULL;
    doc->items = seq_new_ref(PValue);
    doc->sources = seq_new_ref(char *);
    return doc;
}

static ModuleDoc *module_for(ModuleDoc ***modules, const char *name) {
    Seq *seq = (Seq *)modules;
    ModuleDoc **arr = seq->arr;

    FOR (i, array_len(arr)) {
        if (str_eq(arr[i]->name, name))
            return arr[i];
    }
    ModuleDoc *doc = module_doc_new(name);
    seq_add(modules, doc);
    return doc;
}

static void module_set_name_if_empty(ModuleDoc *doc, const char *name) {
    SMap m = doc->module;

    if (m && !*record_string(m, "name"))
        record_update_str(&doc->module, "name", name);
}

static PValue module_value(const char *doc) {
    char ***module = smap_new(true);
    scoped char *name = module_name_from_doc(doc);
    scoped char *clean_doc = doc_without_directives(doc);

    record_put_str(module, "name", name ? name : "");
    record_put_str(module, "doc", clean_doc);
    return smap_close(module);
}

static void merge_module_doc(PValue *module, const char *doc) {
    scoped char *name = module_name_from_doc(doc);
    scoped char *clean_doc = doc_without_directives(doc);
    bool submodule_only = has_directive(doc, "@submodule") &&
                          !has_directive(doc, "@module");
    SMap m = *module;

    if (!m) {
        *module = module_value(doc);
        return;
    }
    if (name && !*record_string(m, "name"))
        record_update_str(module, "name", name);
    if (*clean_doc) {
        const char *old_doc = record_string(m, "doc");
        char **merged;

        if (submodule_only && *old_doc)
            return;
        merged = strbuf_new();
        if (*old_doc) {
            strbuf_adds(merged, old_doc);
            strbuf_adds(merged, "\n\n");
        }
        strbuf_adds(merged, clean_doc);
        scoped char *text = strbuf_tostring(merged);
        record_update_str(module, "doc", text);
    }
}

static PValue item_value(const char *doc, const char *signature) {
    scoped char *table = find_directive_value(doc, "@table");
    scoped char *within = find_directive_value(doc, "@within");
    scoped char *section = find_directive_value(doc, "@section");
    char ***item = smap_new(true);

    if (table) {
        const char *name = table;
        scoped char *clean_doc = doc_without_directives(doc);

        while (*name == '@')
            name++;
        record_put_str(item, "kind", "table");
        record_put_str(item, "name", name);
        smap_put(item, str_ref("fields"), table_fields(doc));
        record_put_str(item, "doc", clean_doc);
    } else {
        const char *args_start;
        scoped char *name = extract_function_name(signature, &args_start);
        scoped char *clean_signature = trim_signature(signature, args_start);
        scoped char *clean_doc = doc_without_directives(doc);

        record_put_str(item, "kind", "function");
        record_put_str(item, "name", name);
        record_put_str(item, "signature", clean_signature);
        if (within)
            record_put_str(item, "within", within);
        if (section)
            record_put_str(item, "section", section);
        record_put_str(item, "doc", clean_doc);
    }
    return smap_close(item);
}

static const char *record_string(SMap m, const char *key) {
    char *value = str_gets(m, key);
    return value ? value : "";
}

static bool item_is_kind(SMap item, const char *kind) {
    return str_eq(record_string(item, "kind"), kind);
}

static void markdown_add_doc(char **out, const char *doc) {
    scoped char *clean = str_trim_range(doc, -1);

    if (*clean) {
        strbuf_adds(out, clean);
        strbuf_adds(out, "\n\n");
    }
}

static void markdown_add_cell(char **out, const char *text) {
    const char *p = text;

    while (*p) {
        if (*p == '|') {
            strbuf_adds(out, "\\|");
        } else if (*p == '\n' || *p == '\r') {
            strbuf_add(out, ' ');
        } else {
            strbuf_add(out, *p);
        }
        p++;
    }
}

static void markdown_add_table(char **out, SMap item) {
    PValue *fields = str_lookup(item, "fields");

    strbuf_addf(out, "### %s\n\n", record_string(item, "name"));
    markdown_add_doc(out, record_string(item, "doc"));
    if (!fields || array_len(fields) == 0)
        return;

    strbuf_adds(out, "| Type | Name | Description |\n");
    strbuf_adds(out, "| --- | --- | --- |\n");
    FOR (i, array_len(fields)) {
        SMap field = fields[i];

        strbuf_adds(out, "| ");
        markdown_add_cell(out, record_string(field, "type"));
        strbuf_adds(out, " | ");
        markdown_add_cell(out, record_string(field, "name"));
        strbuf_adds(out, " | ");
        markdown_add_cell(out, record_string(field, "doc"));
        strbuf_adds(out, " |\n");
    }
    strbuf_add(out, '\n');
}

static void markdown_add_function(char **out, SMap item) {
    strbuf_addf(out, "### %s\n\n", record_string(item, "name"));
    strbuf_adds(out, "```c\n");
    strbuf_adds(out, record_string(item, "signature"));
    strbuf_adds(out, "\n```\n\n");
    markdown_add_doc(out, record_string(item, "doc"));
}

static const char *section_key(SMap item) {
    if (item_is_kind(item, "table"))
        return "types";
    if (item_is_kind(item, "function")) {
        const char *within = record_string(item, "within");

        return *within ? within : "uncategorized";
    }
    return "";
}

static const char *section_title(const char *key) {
    if (str_eq(key, "types"))
        return "Tables";
    if (str_eq(key, "uncategorized"))
        return "Functions";
    return key;
}

typedef struct {
    char *key;
    PValue **items;
} Section;

static void section_dispose(Section *section) {
    obj_unref(section->key);
    obj_unref(section->items);
}

static Section *section_new(const char *key) {
    Section *section = obj_new(Section, section_dispose);

    section->key = str_new(key);
    section->items = seq_new(PValue);
    return section;
}

static Section *section_for(Section ***sections, const char *key) {
    Seq *seq = (Seq *)sections;
    Section **arr = seq->arr;

    FOR (i, array_len(arr)) {
        if (str_eq(arr[i]->key, key))
            return arr[i];
    }

    Section *section = section_new(key);
    seq_add(sections, section);
    return section;
}

static Section ***section_list(PValue *items) {
    Section ***sections = seq_new_ref(Section *);

    if (!items)
        return sections;

    FOR (i, array_len(items)) {
        SMap item = items[i];
        const char *key = section_key(item);

        if (*key) {
            Section *section = section_for(sections, key);
            seq_add(section->items, item);
        }
    }
    return sections;
}

static void markdown_add_section(char **out, Section *section) {
    Seq *bucket_seq = (Seq *)section->items;
    PValue *items = bucket_seq->arr;

    if (array_len(items) == 0)
        return;

    strbuf_addf(out, "## %s\n\n", section_title(section->key));
    FOR (i, array_len(items)) {
        SMap item = items[i];

        if (item_is_kind(item, "table"))
            markdown_add_table(out, item);
        else if (item_is_kind(item, "function"))
            markdown_add_function(out, item);
    }
}

static void markdown_add_section_key(char **out, Section **sections,
                                     const char *key) {
    FOR (i, array_len(sections)) {
        Section *section = sections[i];

        if (str_eq(section->key, key)) {
            markdown_add_section(out, section);
            return;
        }
    }
}

static char *markdown_tostring(SMap root) {
    SMap module = str_lookup(root, "module");
    PValue *items = str_lookup(root, "items");
    char **out = strbuf_new();
    const char *module_name = module ? record_string(module, "name") : "";
    const char *title = *module_name ? module_name : record_string(root, "source");
    scoped Section ***sections = NULL;

    strbuf_addf(out, "# %s\n\n", title);
    if (module)
        markdown_add_doc(out, record_string(module, "doc"));
    if (!items)
        return strbuf_tostring(out);

    sections = section_list(items);
    markdown_add_section_key(out, *sections, "types");
    markdown_add_section_key(out, *sections, "uncategorized");
    FOR (i, array_len(*sections)) {
        Section *section = (*sections)[i];
        const char *key = section->key;

        if (!str_eq(key, "types") && !str_eq(key, "uncategorized"))
            markdown_add_section(out, section);
    }

    return strbuf_tostring(out);
}

static char *read_module_doc(FILE *f, char *line, size_t line_size) {
    char **doc = strbuf_new();

    while (fgets(line, line_size, f)) {
        char *p = (char *)skip_space(line);
        if (strncmp(p, "*/", 2) == 0)
            break;
        strbuf_adds(doc, line);
    }
    return strbuf_tostring(doc);
}

static char *read_line_doc(FILE *f, char *line, size_t line_size,
                           char **signature) {
    char **doc = strbuf_new();
    char *p = (char *)skip_space(line);
    char *doc_text;

    append_doc_line(doc, p + 3);
    while (fgets(line, line_size, f)) {
        p = (char *)skip_space(line);
        if (strncmp(p, "//", 2) != 0)
            break;
        append_doc_line(doc, p + 2);
    }

    doc_text = strbuf_tostring(doc);
    if (has_directive(doc_text, "@table")) {
        *signature = str_new("");
    } else {
        trim_right(p);
        *signature = *p ? str_new(p) : next_signature(f, line, line_size);
    }
    return doc_text;
}

static bool signature_names_function(const char *signature) {
    const char *args_start;
    scoped char *name = extract_function_name(signature, &args_start);

    return *name != '\0';
}

static bool read_source_modules(const char *source, ModuleDoc ***modules) {
    FILE *f = fopen(source, "r");
    char line[LINE_SIZE];
    scoped char *fallback_name = path_basename_no_ext(source);
    scoped char *current_name = NULL;
    ModuleDoc *current = NULL;

    if (!f) {
        perror(source);
        return false;
    }

    while (fgets(line, sizeof(line), f)) {
        char *p = (char *)skip_space(line);

        if (strncmp(p, "/***", 4) == 0) {
            scoped char *doc = read_module_doc(f, line, sizeof(line));
            scoped char *name = module_name_from_doc(doc);

            if (name) {
                obj_unref(current_name);
                current_name = str_new(name);
                current = module_for(modules, current_name);
                merge_module_doc(&current->module, doc);
                seq_add(current->sources, str_new(source));
            } else if (!current) {
                current_name = str_new(fallback_name);
                current = module_for(modules, current_name);
                merge_module_doc(&current->module, doc);
                module_set_name_if_empty(current, current_name);
                seq_add(current->sources, str_new(source));
            }
        } else if (strncmp(p, "///", 3) == 0) {
            scoped char *signature = NULL;
            scoped char *doc = read_line_doc(f, line, sizeof(line), &signature);
            scoped char *name = module_name_from_doc(doc);

            if (name) {
                obj_unref(current_name);
                current_name = str_new(name);
                current = module_for(modules, current_name);
                merge_module_doc(&current->module, doc);
                seq_add(current->sources, str_new(source));
            } else {
                if (!current) {
                    current_name = str_new(fallback_name);
                    current = module_for(modules, current_name);
                    merge_module_doc(&current->module, "");
                    module_set_name_if_empty(current, current_name);
                    seq_add(current->sources, str_new(source));
                }
                if (has_directive(doc, "@table") ||
                    signature_names_function(signature)) {
                    seq_add(current->items, item_value(doc, signature));
                } else {
                    merge_module_doc(&current->module, doc);
                }
            }
        }
    }
    fclose(f);
    return true;
}

static SMap module_root(ModuleDoc *doc, const char *source_name) {
    char ***root = smap_new(false);
    Seq *items = (Seq *)doc->items;
    Seq *sources = (Seq *)doc->sources;

    smap_add(root, "source", source_name);
    smap_add(root, "sources", sources ? sources->arr : NULL);
    smap_add(root, "module", doc->module);
    smap_add(root, "items", items ? items->arr : NULL);
    return smap_close(root);
}

static bool write_text_file(const char *path, const char *text) {
    if (!file_write_fmt(path, "%s", text)) {
        fprintf(stderr, "cannot write %s\n", path);
        return false;
    }
    return true;
}

static bool write_module_markdown(const char *outdir, const char *name,
                                  ModuleDoc *doc) {
    scoped char *filename = safe_markdown_name(name);
    scoped char *path = file_path_join(outdir, filename);
    scoped SMap root = module_root(doc, name);
    scoped char *text = markdown_tostring(root);

    return write_text_file(path, text);
}

static bool generate_all_markdown(const char *outdir, int root_count,
                                  char **roots) {
    scoped char ***sources = seq_new_ref(char *);
    scoped ModuleDoc ***modules = seq_new_ref(ModuleDoc *);
    const char *default_roots[] = {"llib", "llib-p"};
    int count = root_count > 0 ? root_count : 2;
    int written = 0;

    if (!mkdir_p(outdir))
        return false;

    for (int i = 0; i < count; i++) {
        const char *root = root_count > 0 ? roots[i] : default_roots[i];

        if (!collect_sources(root, sources))
            return false;
    }
    qsort((*sources), array_len(*sources), sizeof(char *), cmp_str_ptr);

    FOR (i, array_len(*sources)) {
        if (!read_source_modules((*sources)[i], modules))
            return false;
    }

    Seq *module_seq = (Seq *)modules;
    ModuleDoc **module_arr = module_seq->arr;

    FOR (i, array_len(module_arr)) {
        ModuleDoc *doc = module_arr[i];

        if (!write_module_markdown(outdir, doc->name, doc))
            return false;
        written++;
    }
    fprintf(stderr, "wrote %d module(s) to %s\n", written, outdir);
    return true;
}

static bool read_source(const char *source, PValue *module, PValue **items) {
    FILE *f = fopen(source, "r");
    char line[LINE_SIZE];

    if (!f) {
        perror(source);
        return false;
    }

    while (fgets(line, sizeof(line), f)) {
        char *p = (char *)skip_space(line);

        if (strncmp(p, "/***", 4) == 0) {
            scoped char *doc = read_module_doc(f, line, sizeof(line));

            if (has_directive(doc, "@module") || !*module)
                merge_module_doc(module, doc);
        } else if (strncmp(p, "///", 3) == 0) {
            scoped char *signature = NULL;
            scoped char *doc = read_line_doc(f, line, sizeof(line), &signature);

            if (has_directive(doc, "@module") ||
                has_directive(doc, "@submodule")) {
                merge_module_doc(module, doc);
            } else {
                seq_add(items, item_value(doc, signature));
            }
        }
    }
    fclose(f);
    return true;
}

static bool parse_args(int argc, char **argv, OutputFormat *format,
                       int *first_source) {
    *format = OUT_MARKDOWN;
    *first_source = 1;
    if (argc < 2)
        return false;
    if (str_eq(argv[1], "--json") || str_eq(argv[1], "-j")) {
        *format = OUT_JSON;
        *first_source = 2;
    } else if (str_eq(argv[1], "--markdown") || str_eq(argv[1], "-m")) {
        *first_source = 2;
    } else if (*argv[1] == '-') {
        return false;
    }
    return *first_source < argc;
}

int main(int argc, char **argv) {
    PValue **items = seq_new_ref(PValue);
    char ***sources = seq_new_ref(char *);
    PValue module = NULL;
    char ***root;
    SMap root_value;
    char *out;
    OutputFormat format;
    int first_source;
    int source_count;

    if (argc >= 3 && str_eq(argv[1], "--all")) {
        return generate_all_markdown(argv[2], argc - 3, argv + 3) ? 0 : 1;
    }

    if (!parse_args(argc, argv, &format, &first_source)) {
        fprintf(stderr,
                "usage: %s [--markdown|--json] file.c ...\n"
                "       %s --all outdir [source-root ...]\n",
                argv[0], argv[0]);
        return 1;
    }

    source_count = argc - first_source;
    for (int i = first_source; i < argc; i++) {
        seq_add(sources, str_new(argv[i]));
        if (!read_source(argv[i], &module, items)) {
            obj_unref(sources);
            obj_unref(items);
            obj_unref(module);
            return 1;
        }
    }

    root = smap_new(true);
    record_put_str(root, "source", source_count == 1 ? argv[first_source] : "");
    smap_put(root, str_ref("sources"), seq_array_ref(sources));
    smap_put(root, str_ref("module"), module);
    smap_put(root, str_ref("items"), seq_array_ref(items));
    root_value = smap_close(root);

    out = format == OUT_JSON ? json_to_pretty_string(root_value, 1)
                             : markdown_tostring(root_value);
    puts(out);
    obj_unref(out);
    obj_unref(root_value);
    return 0;
}
