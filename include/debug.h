#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1

#if DEBUG
#define debug_log(fmt, ...)                                                                                            \
  fprintf(stdout, "[DEBUG] %s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug_log(fmt, ...) // nothing
#endif

#endif // DEBUG_H

