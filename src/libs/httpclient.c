
#include "../libs/http/HTTPClient.h"

#include <curl/curl.h>

struct curlres {
    char *ptr;
    size_t len;
};

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct curlres *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

char *request(char *method, char *host, char *path) {
    CURL *curl;
    CURLcode res;

    char url[1028];
    sprintf(url, "https://%s%s", host, path);

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *chunk = NULL;

        /* Remove a header curl would otherwise add by itself */
        chunk = curl_slist_append(chunk, "Accept: application/json");

        /* Add a custom header */
        // chunk = curl_slist_append(chunk, "Another: yes");

        /* Modify a header curl otherwise adds differently */
        // chunk = curl_slist_append(chunk, "Host: ");
        // chunk = curl_slist_append(chunk, host);

        chunk = curl_slist_append(chunk, "User-Agent: ki package manager");

        /* set our custom set of headers */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // curl_easy_setopt(curl, CURLOPT_URL, "localhost");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        struct curlres cres;
        cres.len = 0;
        cres.ptr = malloc(1);
        cres.ptr[0] = '\0';

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cres);

        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        /* always cleanup */
        curl_easy_cleanup(curl);

        /* free the custom headers */
        curl_slist_free_all(chunk);

        if (res == CURLE_OK) {
            return strdup(cres.ptr);
        }
    }

    return NULL;
}
