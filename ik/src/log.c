#include "cstructures/memory.h"
#include "cstructures/vector.h"
#include "ik/init.h"
#include "ik/callbacks.h"
#include "ik/log.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct log_t
{
    struct vector_t message_buffer;
    uint8_t severity;
    char* prefix;
    int8_t timestamps;
} log_t;

static log_t* g_log = NULL;
static int g_init_counter = 0;

/* ------------------------------------------------------------------------- */
ikret_t
ik_log_init(void)
{
    ikret_t result;

    if (g_init_counter++ != 0)
        return IK_OK;

    /* The log depends on the ik library being initialized */
    if ((result = ik_init()) != IK_OK)
        goto ik_init_failed;

    g_log = (log_t*)MALLOC(sizeof *g_log);
    if (g_log == NULL)
    {
        result = IK_ERR_OUT_OF_MEMORY;
        goto alloc_log_failed;
    }

    vector_init(&g_log->message_buffer, sizeof(char));
    g_log->prefix = NULL;
    g_log->timestamps = 1;
#ifdef DEBUG
    ik_log_severity(IK_LOG_DEVEL);
#else
    ik_log_severity(IK_LOG_INFO);
#endif

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)
    ik_log_prefix(STR(IKAPI));

    return IK_OK;

    alloc_log_failed : ik_deinit();
    ik_init_failed   : return result;
}

/* ------------------------------------------------------------------------- */
void
ik_log_deinit(void)
{
    if (--g_init_counter != 0)
        return;

    vector_deinit(&g_log->message_buffer);
    if (g_log->prefix != NULL)
        FREE(g_log->prefix);
    FREE(g_log);
    g_log = NULL;
    ik_deinit();
}

/* ------------------------------------------------------------------------- */
void
ik_log_severity(enum ik_log_severity_e severity)
{
    g_log->severity = severity;
}

/* ------------------------------------------------------------------------- */
void
ik_log_timestamps(int enable)
{
    g_log->timestamps = (enable != 0);
}

/* ------------------------------------------------------------------------- */
void
ik_log_prefix(const char* prefix)
{
    char* buf;
    int len;

    if (g_log->prefix != NULL)
        FREE(g_log->prefix);
    g_log->prefix = NULL;

    if (prefix == NULL)
        return;

    len = strlen(prefix);
    if (len == 0)
        return;

    buf = MALLOC((sizeof(char) + 1) * len);
    if (buf == NULL)
        return;
    strcpy(buf, prefix);
    g_log->prefix = buf;
}

/* ------------------------------------------------------------------------- */
static const char* severities[] = {
#define X(arg) "[" #arg "]",
    IK_LOG_SEVERITY_LIST
#undef X
};
static void
log_message(enum ik_log_severity_e severity, const char* fmt, va_list vargs)
{
    uintptr_t msg_len;
    char timestamp[12];
    char* buf_ptr;
    va_list vargs_copy;

    if (g_log->timestamps)
    {
        time_t rawtime = time(NULL); /* get system time */
        struct tm* timeinfo = localtime(&rawtime); /* convert to local time */
        strftime(timestamp, 12, "[%X]", timeinfo);
    }

    /* Deterine total length of message */
    va_copy(vargs_copy, vargs);
    msg_len = vsnprintf(NULL, 0, fmt, vargs_copy);
    va_end(vargs_copy);
    if (g_log->prefix)
        msg_len += strlen(g_log->prefix) + 3;
    msg_len += 10; /* for tag */
    msg_len += 12; /* for timestamp */
    msg_len += 2;  /* for 2 spaces */

    if (vector_resize(&g_log->message_buffer, (msg_len + 1) * sizeof(char)) < 0)
        return;

    buf_ptr = (char*)g_log->message_buffer.data;
    if (g_log->prefix)
        buf_ptr += sprintf(buf_ptr, "[%s]", g_log->prefix);
    if (g_log->timestamps)
        buf_ptr += sprintf(buf_ptr, "%s", timestamp);
    buf_ptr += sprintf(buf_ptr, "%s ", severities[severity]);
    vsprintf(buf_ptr, fmt, vargs);

    ik_callbacks_notify_log_message((char*)g_log->message_buffer.data);
}

/* ------------------------------------------------------------------------- */
void
ik_log_devel(const char* fmt, ...)
{
    va_list vargs;

    if (g_log == NULL || g_log->severity > IK_LOG_DEVEL)
        return;

    va_start(vargs, fmt);
    log_message(IK_LOG_DEVEL, fmt, vargs);
    va_end(vargs);
}

/* ------------------------------------------------------------------------- */
void
ik_log_info(const char* fmt, ...)
{
    va_list vargs;

    if (g_log == NULL || g_log->severity > IK_LOG_INFO)
        return;

    va_start(vargs, fmt);
    log_message(IK_LOG_INFO, fmt, vargs);
    va_end(vargs);
}

/* ------------------------------------------------------------------------- */
void
ik_log_warning(const char* fmt, ...)
{
    va_list vargs;

    if (g_log == NULL || g_log->severity > IK_LOG_WARNING)
        return;

    va_start(vargs, fmt);
    log_message(IK_LOG_WARNING, fmt, vargs);
    va_end(vargs);
}

/* ------------------------------------------------------------------------- */
void
ik_log_error(const char* fmt, ...)
{
    va_list vargs;

    if (g_log == NULL || g_log->severity > IK_LOG_ERROR)
        return;

    va_start(vargs, fmt);
    log_message(IK_LOG_ERROR, fmt, vargs);
    va_end(vargs);
}

/* ------------------------------------------------------------------------- */
void
ik_log_fatal(const char* fmt, ...)
{
    va_list vargs;

    if (g_log == NULL || g_log->severity > IK_LOG_FATAL)
        return;

    va_start(vargs, fmt);
    log_message(IK_LOG_FATAL, fmt, vargs);
    va_end(vargs);
}

