#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "lambda.h"

static size_t write_res_cb(char *data, size_t size, size_t nmemb, char *res);
static size_t drop_res_cb(char *data, size_t size, size_t nmemb, void *res);

int main() {
	CURL *curl;
	int res = curl_global_init(CURL_GLOBAL_NOTHING);
	if (res != EXIT_SUCCESS) return res;

	const char *api_host = getenv("AWS_LAMBDA_RUNTIME_API");

	char url[512];
	struct curl_header *header;
	char *request_id;
	char *trace_id;
	char *event = malloc(LAMBDA_MAX_REQUEST_BYTES);
	char *response = malloc(LAMBDA_MAX_RESPONSE_BYTES);
	size_t response_len;

	curl = curl_easy_init();
	while (true) {
		memset(event, 0, LAMBDA_MAX_REQUEST_BYTES);
		memset(response, 0, LAMBDA_MAX_RESPONSE_BYTES);
		curl_easy_reset(curl);

		snprintf(url, sizeof(url), "http://%s/2018-06-01/runtime/invocation/next", api_host);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_res_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, event);
		curl_easy_perform(curl);

		curl_easy_header(curl, "Lambda-Runtime-Aws-Request-Id", 0, CURLH_HEADER, -1, &header);
		request_id = strdup(header->value);

		curl_easy_header(curl, "Lambda-Runtime-Trace-Id", 0, CURLH_HEADER, -1, &header);
		trace_id = strdup(header->value);
		setenv("_X_AMZN_TRACE_ID", trace_id, 1);

		curl_easy_reset(curl);

		struct lambda_context context = {
			.aws_request_id = request_id
		};

		response_len = LAMBDA_MAX_RESPONSE_BYTES;
		res = handler(event, &context, response, &response_len);

		if (res != EXIT_SUCCESS) {
			snprintf(url, sizeof(url), "http://%s/2018-06-01/runtime/invocation/%s/error", api_host, request_id);
			curl_easy_setopt(curl, CURLOPT_POST, 1);
		} else {
			snprintf(url, sizeof(url), "http://%s/2018-06-01/runtime/invocation/%s/response", api_host, request_id);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, response);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, response_len);
		}

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, drop_res_cb);
		curl_easy_perform(curl);

		free(request_id);
		free(trace_id);
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	free(event);
	free(response);
	return EXIT_SUCCESS;
}

static size_t write_res_cb(char *data, size_t size, size_t nmemb, char *res) {
	size_t realsize = size * nmemb;
	size_t curr_len = strlen(res);
	if (curr_len + realsize >= LAMBDA_MAX_REQUEST_BYTES) return 0;

	memcpy(res + curr_len, data, realsize);
	res[curr_len + realsize] = '\0';
	return realsize;
}

static size_t drop_res_cb(char *data, size_t size, size_t nmemb, void *res) {
	return size * nmemb;
}
