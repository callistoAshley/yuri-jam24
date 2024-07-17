#include "interpreter.h"

// TODO: would be nice to extract this function into a separate file of helper functions
static long flen(FILE *file)
{
    long file_len, cur_pos;
    cur_pos = ftell(file);

    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, cur_pos, SEEK_SET);

    return file_len;
}

static bool is_newline(char c)
{
    return c == '\n' || c == '\r';
}

static Event construct_event(char *name, char *text)
{
    Event event;
    Token token;
    char err_msg[256];
    size_t num_read;

    memset(err_msg, 0, 256);

    event.name = malloc(strlen(name + 1));
    strcpy(event.name, name);
    event.tokens = calloc(1, sizeof(Token));
    event.num_tokens = 0;

    while (num_read = lexer_next_token(text, &token, err_msg), token.type != TOKEN_NONE)
    {
        if (err_msg[0])
        {
            free(event.name);
            free(event.tokens);
            FATAL("ERROR: construct_event: %s\n", err_msg);
        }

        text += num_read;
        event.tokens = realloc(event.tokens, ++event.num_tokens * sizeof(Token)); // TODO: check for memory leaks here
        memcpy(event.tokens + (event.num_tokens - 1), &token, sizeof(Token)); // FIXME: is the memcpy really necessary?
    }

    return event;
}

static Event *get_events(char *text, int *out_num_events)
{
    Event *result;
#define UPDATE_RESULT \
    Event event = construct_event(event_name, event_text); \
    result = realloc(result, ++(*out_num_events) * sizeof(Event)); /* TODO: check for memory leaks here */ \
    result[(*out_num_events) - 1] = event;

    char event_name[256] = "NONE";
    char *event_text = NULL;
    int event_text_len, event_text_idx;
    bool newline = true;

    *out_num_events = 0;
    result = calloc(1, sizeof(Event));

    while (*text)
    {
        // new event
        if (newline && *text == '=')
        {
            int name_len;

            if (event_text)
            {
                UPDATE_RESULT;
            }

            // construct new event name
            for (name_len = 0; !is_newline(*text) && *text; name_len++, text++)
            {
                event_name[name_len] = *text;
            }
            event_name[name_len] = '\0';

            event_text_idx = 0;
            event_text_len = 1;
            event_text = malloc(event_text_len);
        }

        if (event_text)
        {
            event_text = realloc(event_text, ++event_text_len); // TODO: check for memory leaks here
            event_text[event_text_idx++] = *text;
            event_text[event_text_idx] = '\0';
        }

        newline = is_newline(*text);
        text++;
    }

    UPDATE_RESULT;

    return result;
}

Interpreter *interpreter_init(char **files, int num_files)
{
    Interpreter *interpreter = calloc(1, sizeof(Interpreter));
    if (!interpreter) return NULL;

    interpreter->events = linked_list_init();

    for (int i = 0; i < num_files; i++)
    {
        Event *events;
        int num_events;

        char *file_text;
        long file_len;
        char *file_path = files[i];
        FILE *file = fopen(file_path, "r");

        file_len = flen(file);
        file_text = malloc(file_len);
        if (!file_text)
        {
            fclose(file);
            interpreter_free(interpreter);
            return NULL;
        }
        fread(file_text, 1, file_len, file);

        events = get_events(file_text, &num_events);
        printf("num_events: %d\n", num_events);
        for (int j = 0; j < num_events; j++)
        {
            Event *event = malloc(sizeof(Event));
            memcpy(event, events + j, sizeof(Event));
            linked_list_append(interpreter->events, event);

            printf("%s:\n", event->name);
            for (int tk = 0; tk < event->num_tokens; tk++)
            {
                printf("  ");
                switch (event->tokens[tk].type)
                {
                    case TOKEN_NONE:
                        printf("(NONE)");
                        break;
                    case TOKEN_TEXT:
                        printf("(TEXT): %s", event->tokens[tk].text);
                        break;
                    case TOKEN_CMD:
                        printf("(CMD): %s|%s||", event->tokens[tk].command.name, event->tokens[tk].command.arg);
                        break;
                }
                printf("\n");
            }
        }
        free(events);

        free(file_text);
        fclose(file);
    }

    return interpreter;
}

void interpreter_free(Interpreter *interpreter)
{
    for (LinkedListNode *node = interpreter->events->first; node; node = node->next)
    {
        Event *event = node->data;
        free(event->name);
        free(event->tokens);
        free(event);
    }
    linked_list_free(interpreter->events);
    free(interpreter);
}
