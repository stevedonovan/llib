#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 4096

typedef struct {
    char *text;
    size_t len;
    size_t cap;
} String;

static void die(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

static char *xstrdup(const char *s) {
    char *res = malloc(strlen(s) + 1);
    if (!res)
        die("out of memory");
    strcpy(res, s);
    return res;
}

static void string_init(String *s) {
    s->cap = 256;
    s->len = 0;
    s->text = malloc(s->cap);
    if (!s->text)
        die("out of memory");
    s->text[0] = '\0';
}

static void string_free(String *s) { free(s->text); }

static void string_reserve(String *s, size_t extra) {
    size_t need = s->len + extra + 1;
    if (need <= s->cap)
        return;
    while (s->cap < need)
        s->cap *= 2;
    s->text = realloc(s->text, s->cap);
    if (!s->text)
        die("out of memory");
}

static void string_addn(String *s, const char *text, size_t n) {
    string_reserve(s, n);
    memcpy(s->text + s->len, text, n);
    s->len += n;
    s->text[s->len] = '\0';
}

static void string_adds(String *s, const char *text) {
    string_addn(s, text, strlen(text));
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

static char *trimmed_dup(const char *start, size_t len) {
    while (len > 0 && isspace((unsigned char)*start)) {
        start++;
        len--;
    }
    while (len > 0 && isspace((unsigned char)start[len - 1]))
        len--;
    char *res = malloc(len + 1);
    if (!res)
        die("out of memory");
    memcpy(res, start, len);
    res[len] = '\0';
    return res;
}

static char *find_directive_value(const char *doc, const char *directive) {
    size_t dlen = strlen(directive);
    const char *p = doc;
    while ((p = strstr(p, directive)) != NULL) {
        if ((p == doc || p[-1] == '\n' || isspace((unsigned char)p[-1])) &&
            (p[dlen] == '\0' || isspace((unsigned char)p[dlen]))) {
            const char *value = skip_space(p + dlen);
            const char *end = strchr(value, '\n');
            if (!end)
                end = value + strlen(value);
            return trimmed_dup(value, (size_t)(end - value));
        }
        p += dlen;
    }
    return NULL;
}

static int has_directive(const char *doc, const char *directive) {
    char *value = find_directive_value(doc, directive);
    if (value) {
        free(value);
        return 1;
    }
    return 0;
}

static char *extract_function_name(const char *sig) {
    const char *paren = strchr(sig, '(');
    const char *end;
    const char *start;

    if (!paren)
        return xstrdup("");
    end = paren;
    while (end > sig && isspace((unsigned char)end[-1]))
        end--;
    start = end;
    while (start > sig &&
           (isalnum((unsigned char)start[-1]) || start[-1] == '_'))
        start--;
    return trimmed_dup(start, (size_t)(end - start));
}

static char *next_signature(FILE *f, char *line, size_t line_size) {
    while (fgets(line, line_size, f)) {
        char *p = (char *)skip_space(line);
        if (*p == '\0')
            continue;
        trim_right(p);
        return xstrdup(p);
    }
    return xstrdup("");
}

static void json_string(const char *s) {
    putchar('"');
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        switch (ch) {
        case '\\':
            fputs("\\\\", stdout);
            break;
        case '"':
            fputs("\\\"", stdout);
            break;
        case '\n':
            fputs("\\n", stdout);
            break;
        case '\r':
            fputs("\\r", stdout);
            break;
        case '\t':
            fputs("\\t", stdout);
            break;
        default:
            if (ch < 32)
                printf("\\u%04x", ch);
            else
                putchar(ch);
            break;
        }
    }
    putchar('"');
}

static void print_json_field(const char *name, const char *value, int comma) {
    printf("    ");
    json_string(name);
    printf(": ");
    json_string(value ? value : "");
    if (comma)
        putchar(',');
    putchar('\n');
}

static void append_doc_line(String *doc, const char *p) {
    if (*p == ' ')
        p++;
    string_adds(doc, p);
}

static void print_table_fields(const char *doc) {
    const char *line = doc;
    int first = 1;

    printf("    \"fields\": [");
    while (*line) {
        const char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) : strlen(line);
        const char *p = line;

        while (len > 0 && isspace((unsigned char)*p)) {
            p++;
            len--;
        }
        if (len > 1 && *p == '@' && isalpha((unsigned char)p[1]) &&
            strncmp(p, "@table", 6) != 0) {
            const char *type = p + 1;
            const char *type_end = type;
            const char *name;
            const char *desc;
            const char *end = p + len;
            char *type_text;
            char *name_text;
            char *desc_text;

            while (type_end < end && !isspace((unsigned char)*type_end))
                type_end++;
            name = type_end;
            while (name < end && isspace((unsigned char)*name))
                name++;
            desc = name;
            while (desc < end && !isspace((unsigned char)*desc))
                desc++;

            if (!first)
                putchar(',');
            printf("\n      {\n");
            printf("        \"type\": ");
            type_text = trimmed_dup(type, (size_t)(type_end - type));
            json_string(type_text);
            printf(",\n        \"name\": ");
            name_text = trimmed_dup(name, (size_t)(desc - name));
            json_string(name_text);
            printf(",\n        \"doc\": ");
            while (desc < end && isspace((unsigned char)*desc))
                desc++;
            desc_text = trimmed_dup(desc, (size_t)(end - desc));
            json_string(desc_text);
            printf("\n      }");
            free(type_text);
            free(name_text);
            free(desc_text);
            first = 0;
        }
        if (!next)
            break;
        line = next + 1;
    }
    if (!first)
        putchar('\n');
    printf("    ],\n");
}

static void print_module(String *doc, int *printed_module) {
    char *name = find_directive_value(doc->text, "@module");

    printf("  \"module\": {\n");
    print_json_field("name", name ? name : "", 1);
    print_json_field("doc", doc->text, 0);
    printf("  },\n");
    *printed_module = 1;
    free(name);
}

static void print_item(String *doc, char *signature, int *printed_module,
                       int *opened_items, int *first_item) {
    char *table = find_directive_value(doc->text, "@table");
    char *within = find_directive_value(doc->text, "@within");
    char *section = find_directive_value(doc->text, "@section");
    char *name;

    if (!*printed_module) {
        printf("  \"module\": null,\n");
        *printed_module = 1;
    }
    if (!*opened_items) {
        printf("  \"items\": [\n");
        *opened_items = 1;
    }

    if (*first_item)
        *first_item = 0;
    else
        printf(",\n");

    if (table) {
        const char *p = table;
        while (*p == '@')
            p++;
        name = xstrdup(p);
        printf("    {\n");
        print_json_field("kind", "table", 1);
        print_json_field("name", name, 1);
        print_table_fields(doc->text);
        print_json_field("doc", doc->text, 0);
        printf("    }");
        free(name);
    } else {
        name = extract_function_name(signature);
        printf("    {\n");
        print_json_field("kind", "function", 1);
        print_json_field("name", name, 1);
        print_json_field("signature", signature, 1);
        if (within)
            print_json_field("within", within, 1);
        if (section)
            print_json_field("section", section, 1);
        print_json_field("doc", doc->text, 0);
        printf("    }");
        free(name);
    }
    free(table);
    free(within);
    free(section);
}

int main(int argc, char **argv) {
    FILE *f;
    char line[LINE_SIZE];
    int printed_module = 0;
    int opened_items = 0;
    int first_item = 1;

    if (argc != 2) {
        fprintf(stderr, "usage: %s file.c\n", argv[0]);
        return 1;
    }

    f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }

    printf("{\n");
    print_json_field("source", argv[1], 1);

    while (fgets(line, sizeof(line), f)) {
        char *p = (char *)skip_space(line);

        if (strncmp(p, "/***", 4) == 0) {
            String doc;
            string_init(&doc);
            while (fgets(line, sizeof(line), f)) {
                p = (char *)skip_space(line);
                if (strncmp(p, "*/", 2) == 0)
                    break;
                string_adds(&doc, line);
            }
            if (has_directive(doc.text, "@module"))
                print_module(&doc, &printed_module);
            string_free(&doc);
        } else if (strncmp(p, "///", 3) == 0) {
            String doc;
            char *signature;
            string_init(&doc);
            append_doc_line(&doc, p + 3);

            while (fgets(line, sizeof(line), f)) {
                p = (char *)skip_space(line);
                if (strncmp(p, "//", 2) != 0)
                    break;
                append_doc_line(&doc, p + 2);
            }

            if (has_directive(doc.text, "@table"))
                signature = xstrdup("");
            else {
                trim_right(p);
                signature = *p ? xstrdup(p) : next_signature(f, line, sizeof(line));
            }
            print_item(&doc, signature, &printed_module, &opened_items,
                       &first_item);
            free(signature);
            string_free(&doc);
        }
    }

    if (!printed_module)
        printf("  \"module\": null,\n");
    if (!opened_items)
        printf("  \"items\": [\n");
    printf("\n  ]\n");
    printf("}\n");

    fclose(f);
    return 0;
}
