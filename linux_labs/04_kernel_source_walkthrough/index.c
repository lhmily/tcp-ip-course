#include "lab.h"

#include <stdio.h>
#include <string.h>

static void tcpip_linux_l04_usage(const char *program) {
  fprintf(stderr, "usage: %s --list | --ingress | --egress | SYMBOL_ID\n", program);
}

static int tcpip_linux_l04_is_option(const char *argument) {
  return argument != NULL && argument[0] == '-';
}

static int tcpip_linux_l04_print_symbol(tcpip_linux_l04_symbol_id id) {
  const tcpip_linux_l04_symbol *symbol = NULL;
  char url[256];
  size_t written = 0U;

  if (tcpip_linux_l04_symbol_by_id(id, &symbol) != TCPIP_LINUX_L04_OK ||
      tcpip_linux_l04_format_source_url(id, url, sizeof(url), &written) != TCPIP_LINUX_L04_OK) {
    return 1;
  }
  (void)written;
  if (printf("%s\t%s\t%s\n", symbol->key, symbol->description, url) < 0) {
    return 1;
  }
  return 0;
}

static int tcpip_linux_l04_print_path(const tcpip_linux_l04_symbol_id *path, size_t count) {
  size_t index;
  if (path == NULL || tcpip_linux_l04_validate_path(path, count) != TCPIP_LINUX_L04_OK) {
    return 1;
  }
  for (index = 0U; index < count; index += 1U) {
    const tcpip_linux_l04_symbol *symbol = NULL;
    if (tcpip_linux_l04_symbol_by_id(path[index], &symbol) != TCPIP_LINUX_L04_OK) {
      return 1;
    }
    if (printf("%s%s", index == 0U ? "" : " -> ", symbol->key) < 0) {
      return 1;
    }
  }
  if (putchar('\n') == EOF || fflush(stdout) == EOF || ferror(stdout) != 0) {
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  size_t index;
  if (argc != 2) {
    tcpip_linux_l04_usage(argv[0]);
    return 2;
  }
  if (strcmp(argv[1], "--list") == 0) {
    for (index = 0U; index < TCPIP_LINUX_L04_SYMBOL_COUNT; index += 1U) {
      if (tcpip_linux_l04_print_symbol((tcpip_linux_l04_symbol_id)index) != 0) {
        return 1;
      }
    }
    return 0;
  }
  if (strcmp(argv[1], "--ingress") == 0) {
    size_t count = 0U;
    const tcpip_linux_l04_symbol_id *path = tcpip_linux_l04_ingress_path(&count);
    return tcpip_linux_l04_print_path(path, count);
  }
  if (strcmp(argv[1], "--egress") == 0) {
    size_t count = 0U;
    const tcpip_linux_l04_symbol_id *path = tcpip_linux_l04_egress_path(&count);
    return tcpip_linux_l04_print_path(path, count);
  }
  if (tcpip_linux_l04_is_option(argv[1])) {
    fprintf(stderr, "unknown option: %s\n", argv[1]);
    tcpip_linux_l04_usage(argv[0]);
    return 2;
  }
  for (index = 0U; index < TCPIP_LINUX_L04_SYMBOL_COUNT; index += 1U) {
    const tcpip_linux_l04_symbol *symbol = NULL;
    if (tcpip_linux_l04_symbol_by_id((tcpip_linux_l04_symbol_id)index, &symbol) ==
            TCPIP_LINUX_L04_OK &&
        strcmp(argv[1], symbol->key) == 0) {
      return tcpip_linux_l04_print_symbol((tcpip_linux_l04_symbol_id)index);
    }
  }
  fprintf(stderr, "unknown symbol ID: %s\n", argv[1]);
  return 2;
}
