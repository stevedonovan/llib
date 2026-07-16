// A tiny note-taking program using llib.
// Notes are stored as individual text files under ~/.noter.

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define mkdir_p(path) _mkdir(path)
#define stat_path(path, st) _stat(path, st)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define mkdir_p(path) mkdir(path, 0700)
#define stat_path(path, st) stat(path, st)
#endif

#include <llib/array.h>
#include <llib/arg.h>
#include <llib/file.h>
#include <llib/str.h>
#include <llib/value.h>

#define PATH_SIZE 512
#define LINE_SIZE 1024
#define INDEX_FILE ".index"
#define ANSI_RESET "\033[0m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_BLUE "\033[94m"

static char *noter_dir;
static const char **new_tags;
static str_t new_body;

static char *notes_dir(void)
{
    return file_expand_user("~/.noter");
}

static int ensure_notes_dir(const char *dir)
{
    if (file_exists(dir, "r"))
        return 0;

    if (mkdir_p(dir) == 0)
        return 0;

    if (errno == EEXIST)
        return 0;

    return 1;
}

static char *slug_from_title(const char *title)
{
    char *slug = str_new_size(strlen(title));
    char *out = slug;
    bool last_dash = false;

    for (const unsigned char *p = (const unsigned char *)title; *p; ++p) {
        if (isalnum(*p)) {
            *out++ = (char)tolower(*p);
            last_dash = false;
        } else if (! last_dash) {
            *out++ = '-';
            last_dash = true;
        }
    }

    while (out > slug && out[-1] == '-')
        --out;

    if (out == slug)
        *out++ = 'n';

    *out = '\0';
    return slug;
}

static char *note_path(const char *dir, const char *title)
{
    char path[PATH_SIZE];
    char *slug = slug_from_title(title);

    snprintf(path, sizeof(path), "%s/%s.note", dir, slug);
    unref(slug);
    return str_new(path);
}

static char *index_path(const char *dir)
{
    char path[PATH_SIZE];

    snprintf(path, sizeof(path), "%s/%s", dir, INDEX_FILE);
    return str_new(path);
}

static bool is_index(const char *s)
{
    if (! *s)
        return false;

    while (*s) {
        if (! isdigit((unsigned char)*s))
            return false;
        ++s;
    }

    return true;
}

static char *read_indexed_path(const char *dir, const char *index_text)
{
    char *path = index_path(dir);
    FILE *f = fopen(path, "r");
    char line[PATH_SIZE];
    int wanted = atoi(index_text);
    int n = 1;

    unref(path);

    if (! f || wanted < 1) {
        if (f)
            fclose(f);
        return NULL;
    }

    while (file_gets(f, line, sizeof(line))) {
        if (n == wanted) {
            fclose(f);
            return str_new(line);
        }
        ++n;
    }

    fclose(f);
    return NULL;
}

static void clear_index(const char *dir)
{
    char *path = index_path(dir);
    FILE *f = fopen(path, "w");

    if (f)
        fclose(f);
    unref(path);
}

static char *read_stdin(void)
{
    char **buf = strbuf_new();
    char line[LINE_SIZE];

    while (fgets(line, sizeof(line), stdin))
        strbuf_adds(buf, line);

    return strbuf_tostring(buf);
}

static char *join_tags(char **tags)
{
    if (! tags || array_len(tags) == 0)
        return str_new("");
    return str_concat(tags, ", ");
}

static char *shell_quote(const char *s)
{
    char **buf = strbuf_new();

    strbuf_adds(buf, "'");
    while (*s) {
        if (*s == '\'')
            strbuf_adds(buf, "'\\''");
        else
            strbuf_add(buf, *s);
        ++s;
    }
    strbuf_adds(buf, "'");
    return strbuf_tostring(buf);
}

static bool tags_contain(const char *tags, const char *tag)
{
    char **parts = str_split(tags, ",");
    bool found = false;

    FOR(i, array_len(parts)) {
        str_trim(parts[i]);
        if (str_case_eq(parts[i], tag)) {
            found = true;
            break;
        }
    }

    unref(parts);
    return found;
}

static int print_note_file(const char *path)
{
    char *note = file_read_all(path, true);

    if (! note) {
        fprintf(stderr, "noter: cannot read %s\n", path);
        return 1;
    }

    puts(note);
    unref(note);
    return 0;
}

static int edit_note_file(const char *path)
{
    const char *editor = getenv("VISUAL");
    char **cmd = strbuf_new();
    char *qpath;
    char *cmd_text;
    int rc;

    if (! editor || ! *editor)
        editor = getenv("EDITOR");
#ifdef _WIN32
    if (! editor || ! *editor)
        editor = "notepad";
#else
    if (! editor || ! *editor)
        editor = "vi";
#endif

    qpath = shell_quote(path);
    strbuf_adds(cmd, editor);
    strbuf_add(cmd, ' ');
    strbuf_adds(cmd, qpath);
    cmd_text = strbuf_tostring(cmd);
    rc = system(cmd_text);
    dispose(qpath, cmd_text);
    return rc;
}

static int read_note_header(const char *path, char *title, int title_len, char *tags, int tags_len)
{
    FILE *f = fopen(path, "r");
    char line[LINE_SIZE];

    *title = '\0';
    *tags = '\0';

    if (! f)
        return 1;

    while (file_gets(f, line, sizeof(line))) {
        if (*line == '\0')
            break;
        if (str_starts_with(line, "Title: ")) {
            snprintf(title, title_len, "%s", line + 7);
        } else if (str_starts_with(line, "Tags: ")) {
            snprintf(tags, tags_len, "%s", line + 6);
        }
    }

    fclose(f);
    return 0;
}

static void format_note_date(time_t t, char *date, int date_len)
{
    struct tm *tm;

    snprintf(date, date_len, "unknown");

    tm = localtime(&t);
    if (! tm)
        return;

    strftime(date, date_len, "%Y-%m-%d", tm);
}

static void note_date(const char *path, char *date, int date_len)
{
    FILE *f = fopen(path, "r");
    char line[LINE_SIZE];
    struct stat st;

    snprintf(date, date_len, "unknown");

    if (f) {
        while (file_gets(f, line, sizeof(line))) {
            if (*line == '\0')
                break;
            if (str_starts_with(line, "Date: ")) {
                snprintf(date, date_len, "%.10s", line + 6);
                fclose(f);
                return;
            }
        }
        fclose(f);
    }

    if (stat_path(path, &st) == 0)
        format_note_date(st.st_mtime, date, date_len);
}

static char **note_files(const char *dir)
{
    char **entries = file_dir_entries(dir);
    char ***files = seq_new_ref(char*);

    FOR(i, entries ? array_len(entries) : 0) {
        if (str_ends_with(entries[i], ".note")) {
            char path[PATH_SIZE];

            snprintf(path, sizeof(path), "%s/%s", dir, entries[i]);
            seq_add(files, str_new(path));
        }
    }

    unref(entries);
    char **res = (char**)seq_array_ref(files);
    array_sort(res, ARRAY_STRING, false, 0);
    return res;
}

static PValue cmd_new(PValue *args)
{
    char *title;
    char *body;
    char *tags_text;
    char *path;
    FILE *f;

    arg_get_values(args, &title);

    if (ensure_notes_dir(noter_dir) != 0)
        return value_errorf("cannot create %s", noter_dir);

    path = note_path(noter_dir, title);
    if (file_exists(path, "r")) {
        unref(path);
        return value_errorf("note already exists: %s", title);
    }

    body = new_body && *new_body ? str_new(new_body) : read_stdin();
    tags_text = join_tags((char**)new_tags);
    f = fopen(path, "w");
    if (! f) {
        PValue err = value_errorf("cannot write %s", path);
        dispose(path, body, tags_text);
        return err;
    }

    fprintf(f, "Title: %s\n", title);
    {
        char date[16];

        format_note_date(time(NULL), date, sizeof(date));
        fprintf(f, "Date: %s\n", date);
    }
    fprintf(f, "Tags: %s\n", tags_text);
    fprintf(f, "\n%s", body);
    if (*body && body[strlen(body) - 1] != '\n')
        fputc('\n', f);

    fclose(f);
    printf("created %s\n", path);
    dispose(path, body, tags_text);
    return NULL;
}

static PValue cmd_show(PValue *args)
{
    char *title;
    char *path;
    bool from_index = false;

    arg_get_values(args, &title);

    if (ensure_notes_dir(noter_dir) != 0)
        return value_errorf("cannot create %s", noter_dir);

    if (is_index(title)) {
        path = read_indexed_path(noter_dir, title);
        if (! path)
            return value_errorf("no note at index '%s'; run list or search", title);
        from_index = true;
    } else {
        path = note_path(noter_dir, title);
    }

    if (print_note_file(path) != 0) {
        unref(path);
        if (from_index)
            return value_errorf("no note at index '%s'", title);
        return value_errorf("no note named '%s'", title);
    }
    unref(path);
    return NULL;
}

static PValue cmd_edit(PValue *args)
{
    char *title;
    char *path;
    bool from_index = false;

    arg_get_values(args, &title);

    if (ensure_notes_dir(noter_dir) != 0)
        return value_errorf("cannot create %s", noter_dir);

    if (is_index(title)) {
        path = read_indexed_path(noter_dir, title);
        if (! path)
            return value_errorf("no note at index '%s'; run list or search", title);
        from_index = true;
    } else {
        path = note_path(noter_dir, title);
    }

    if (! file_exists(path, "r")) {
        unref(path);
        if (from_index)
            return value_errorf("no note at index '%s'", title);
        return value_errorf("no note named '%s'", title);
    }

    if (edit_note_file(path) != 0) {
        PValue err = value_errorf("editor failed for %s", path);
        unref(path);
        return err;
    }

    unref(path);
    return NULL;
}

static PValue cmd_list(PValue *args)
{
    char *tag;
    char **files;
    char *idx_path;
    FILE *idx;
    bool any = false;
    int n = 1;

    arg_get_values(args, &tag);

    if (ensure_notes_dir(noter_dir) != 0)
        return value_errorf("cannot create %s", noter_dir);

    files = note_files(noter_dir);

    if (! files || array_len(files) == 0) {
        puts("no notes");
        clear_index(noter_dir);
        unref(files);
        return NULL;
    }

    idx_path = index_path(noter_dir);
    idx = fopen(idx_path, "w");
    if (! idx) {
        unref(files);
        PValue err = value_errorf("cannot write %s", idx_path);
        unref(idx_path);
        return err;
    }

    for (int i = array_len(files); i > 0; --i) {
        char *path = files[i - 1];
        char title[LINE_SIZE];
        char tags[LINE_SIZE];
        char date[16];

        if (read_note_header(path, title, sizeof(title), tags, sizeof(tags)) == 0) {
            if (*tag && ! tags_contain(tags, tag))
                continue;
            note_date(path, date, sizeof(date));
            printf(ANSI_BRIGHT_YELLOW "%02d" ANSI_RESET "\t"
                   ANSI_BRIGHT_BLUE "%s" ANSI_RESET "\t%s%s%s\n",
                   n, date, title, *tags ? "\t" : "", tags);
            fprintf(idx, "%s\n", path);
            ++n;
            any = true;
        }
    }

    fclose(idx);
    unref(idx_path);

    if (*tag && ! any)
        printf("no notes tagged '%s'\n", tag);

    unref(files);
    return NULL;
}

static PValue cmd_search(PValue *args)
{
    char *pattern;
    char **files;
    char *idx_path;
    FILE *idx;
    char *qpattern;
    bool any = false;
    int n = 1;

    arg_get_values(args, &pattern);

    if (ensure_notes_dir(noter_dir) != 0)
        return value_errorf("cannot create %s", noter_dir);

    files = note_files(noter_dir);
    if (! files || array_len(files) == 0) {
        puts("no notes");
        clear_index(noter_dir);
        unref(files);
        return NULL;
    }

    idx_path = index_path(noter_dir);
    idx = fopen(idx_path, "w");
    if (! idx) {
        unref(files);
        PValue err = value_errorf("cannot write %s", idx_path);
        unref(idx_path);
        return err;
    }

    qpattern = shell_quote(pattern);
    FOR(i, array_len(files)) {
        char **cmd = strbuf_new();
        char *qfile = shell_quote(files[i]);
        char *cmd_text;
        char **matches;
        char title[LINE_SIZE];
        char tags[LINE_SIZE];
        char date[16];

        strbuf_addf(cmd, "egrep --color=always -n -- %s %s", qpattern, qfile);
        cmd_text = strbuf_tostring(cmd);
        matches = file_command_lines(cmd_text);
        if (read_note_header(files[i], title, sizeof(title), tags, sizeof(tags)) != 0
            || ! *title)
            snprintf(title, sizeof(title), "%s", files[i]);
        note_date(files[i], date, sizeof(date));

        FOR(j, matches ? array_len(matches) : 0) {
            printf(ANSI_BRIGHT_YELLOW "%02d" ANSI_RESET "\t"
                   ANSI_BRIGHT_BLUE "%s" ANSI_RESET "\t%s:%s\n",
                   n, date, title, matches[j]);
            fprintf(idx, "%s\n", files[i]);
            ++n;
            any = true;
        }

        dispose(cmd_text, matches);
        unref(qfile);
    }
    unref(qpattern);
    fclose(idx);
    unref(idx_path);

    if (! any)
        printf("no notes matching '%s'\n", pattern);

    unref(files);
    return NULL;
}

PValue noter_args[] = {
    "// noter: simple notes stored in ~/.noter",
    "cmd new(string title); // create a note; reads stdin if --body is empty", cmd_new,
    "cmd show(string title); // show a note by title", cmd_show,
    "cmd edit(string title); // edit a note by title or list/search index", cmd_edit,
    "cmd search(string pattern); // search notes with egrep", cmd_search,
    "cmd list(string tag=''); // list notes, optionally matching a tag", cmd_list,
    "string tags[]; // -t tag for a new note", &new_tags,
    "string body=''; // -b body text for a new note", &new_body,
    NULL
};

int main(int argc, const char **argv)
{
    noter_dir = notes_dir();
    arg_command_line(noter_args, argv);
    unref(noter_dir);
    return 0;
}
