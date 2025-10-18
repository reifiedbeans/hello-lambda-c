#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lambda.h"

int handler(const char *event, struct lambda_context *context, char *response, size_t *response_len) {
	const char* message = "Hello, Lambda!";
	size_t message_len = strlen(message);

	if (*response_len < message_len + 1) return EXIT_FAILURE;
	strcpy(response, message);
	*response_len = message_len;

	return EXIT_SUCCESS;
}
