#include <stddef.h>

#define LAMBDA_MAX_REQUEST_BYTES  (6 * 1024 * 1024)
#define LAMBDA_MAX_RESPONSE_BYTES (6 * 1024 * 1024)

struct lambda_context {
	char *aws_request_id;
};

extern int handler(const char *event, struct lambda_context *context, char *response, size_t *response_len);
