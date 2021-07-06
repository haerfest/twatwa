#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "esp.h"
#include "log.h"


#define MAX_AT_PREFIX_LENGTH  20

#define TX_SIZE  (8 * 1024)
#define RX_SIZE  (8 * 1024)

#define CR       '\r'
#define LF       '\n'
#define CRLF     "\r\n"


typedef void (*tx_handler_t)(void);


typedef struct {
  u8_t*  data;
  size_t size;
  size_t read_index;
  size_t n_elements;
} buffer_t;


static size_t buffer_read(buffer_t* buffer, u8_t* value) {
  if (buffer->n_elements == 0) {
    return 0;
  }

  if (value) {
    *value = buffer->data[buffer->read_index];
  }
  buffer->n_elements--;
  buffer->read_index = (buffer->read_index + 1) % buffer->size;

  return 1;
}


static size_t buffer_read_n(buffer_t* buffer, size_t n, u8_t* values) {
  size_t i;

  for (i = 0; buffer->n_elements && (i < n); i++) {
    if (values) {
      values[i] = buffer->data[buffer->read_index];
    }
    buffer->n_elements--;
    buffer->read_index = (buffer->read_index + 1) % buffer->size;
  }

  return i;
}


static size_t buffer_peek(buffer_t* buffer, size_t index, u8_t* value) {
  if (buffer->n_elements <= index) {
    return 0;
  }

  *value = buffer->data[(buffer->read_index + index) % buffer->size];

  return 1;
}


static size_t buffer_peek_n(buffer_t* buffer, size_t index, size_t n, u8_t* values) {
  size_t i;

  for (i = 0; (index + i < buffer->n_elements) && (i < n); i++) {
    values[i] = buffer->data[(buffer->read_index + index + i) % buffer->size];
  }

  return i;
}


static size_t buffer_write(buffer_t* buffer, u8_t value) {
  if (buffer->n_elements == buffer->size) {
    return 0;
  }

  buffer->data[(buffer->read_index + buffer->n_elements) % buffer->size] = value;
  buffer->n_elements++;

  return 1;
}


typedef struct {
  buffer_t     tx;
  buffer_t     rx;
  tx_handler_t tx_handler;

  int          do_echo;
} esp_t;


static esp_t self;


int esp_init(void) {
  self.tx.size = TX_SIZE;
  self.rx.size = RX_SIZE;
  self.tx.data = (u8_t *) malloc(self.tx.size);
  self.rx.data = (u8_t *) malloc(self.rx.size);

  if (self.tx.data == NULL || self.rx.data == NULL) {
    log_wrn("esp: out of memory\n");
    if (self.tx.data != NULL) free(self.tx.data);
    if (self.rx.data != NULL) free(self.rx.data);
    return 1;
  }

  esp_reset(E_RESET_HARD);

  return 0;
}


void esp_finit(void) {
}


static void respond(const char* response) {
  while (*response) {
    (void) buffer_write(&self.rx, *response++);
  }
}


static void ok(void) {
  respond("OK" CRLF);
}


static void error(void) {
  respond("ERROR" CRLF);
}


static void next(void) {
  u8_t value;

  /* Precondition: there is a CRLF in the TX buffer. */

  while (1) {
    /* Hunt for CR. */
    while (buffer_read(&self.tx, &value) && value != CR);

    /* Could be sole CR, not followed by LF. */
    if (!buffer_read(&self.tx, &value)) {
      return;
    }
    if (value == CR) {
      break;
    }
  }
}


typedef void (*at_handler_t)(void);


/**
 * ATE0
 */
static void at_echo_on(void) {
  self.do_echo = 1;
  ok();
}


/**
 * ATE1
 */
static void at_echo_off(void) {
  self.do_echo = 0;
  ok();
}


static void at(void) {
  /* Order these from longest to shortest prefix. */
  const struct {
    char*        prefix;
    at_handler_t handler;
  } handlers[] = {
    { "E0", at_echo_off },
    { "E1", at_echo_on }
  };
  const size_t n_handlers = sizeof(handlers) / sizeof(*handlers);
  char         prefix[MAX_AT_PREFIX_LENGTH + 1];
  size_t       i, j;

  /* Command syntax must be one of:
   * 1. AT<cmd>?
   * 2. AT<cmd>=<payload>
   * 3. AT<cmd>
   */
  i = buffer_peek_n(&self.tx, 0, MAX_AT_PREFIX_LENGTH, (u8_t *) prefix);
  for (j = 0; (j < i) && (prefix[j] != '=' && prefix[j] != '?' && prefix[j] != CR); j++);
  prefix[j] = 0;

  log_wrn("esp: got '%s'\n", prefix);

  for (i = 0; i < n_handlers; i++) {
    const size_t n = strlen(handlers[i].prefix);
    if (strncmp(prefix, handlers[i].prefix, n) == 0) {
      handlers[i].handler();
      return;
    }
  }

  /* Unknown command. */
  log_wrn("esp: unknown AT-command: 'AT%s'\n", prefix);
  error();
}


static void idle_tx(void) {
  u8_t prefix[2] = { 0, 0 };

  /* Wait for carriage return. */
  if (self.tx.n_elements < 2) {
    return;
  }
  (void) buffer_peek_n(&self.tx, self.tx.n_elements - 2, 2, prefix);
  if (strncmp((const char *) prefix, CRLF, 2) != 0) {
    return;
  }

  /* Echo if required. */
  if (self.do_echo) {
    size_t i;
    for (i = 0; i < self.tx.n_elements; i++) {
      u8_t value;
      (void) buffer_peek(&self.tx, i, &value);
      (void) buffer_write(&self.rx, value);
    }
  }

  /* Must be AT-command. */
  (void) buffer_read_n(&self.tx, 2, prefix);
  if (strncmp((const char*) prefix, "AT", 2) != 0) {
    error();
    next();
    return;
  }

  /* Handle AT-command. */
  at();
  next();
}


void esp_tx_write(u8_t value) {
  if (!buffer_write(&self.tx, value)) {
    /* Buffer full, signal? */
    error();
    return;
  }

  self.tx_handler();
}


u8_t esp_tx_read(void) {
  return (self.tx.n_elements == 0)                    << 4 /* Tx empty      */
       | (self.rx.n_elements >= self.rx.size * 3 / 4) << 3 /* Rx near full  */
       | (self.tx.n_elements == self.tx.size)         << 1 /* Tx full       */
       | (self.rx.n_elements > 0);                         /* Rx not empty  */
}


u8_t esp_rx_read(void) {
  u8_t value;

  return buffer_read(&self.rx, &value) ? value : 0x00;
}


void esp_reset(reset_t reset) {
  self.tx.read_index = 0;
  self.tx.n_elements = 0;
  self.rx.read_index = 0;
  self.rx.n_elements = 0;

  self.tx_handler = idle_tx;

  self.do_echo = 1;
}
