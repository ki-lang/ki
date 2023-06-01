
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

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

        // Headers
        chunk = curl_slist_append(chunk, "Accept: application/json");
        chunk = curl_slist_append(chunk, "User-Agent: ki package manager");
        //
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        struct curlres cres;
        cres.len = 0;
        cres.ptr = "";
        // cres.ptr = malloc(1);
        // cres.ptr[0] = '\0';

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cres);

        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk);

        if (res == CURLE_OK) {
            return strdup(cres.ptr);
        }
    }

    return NULL;
}

char download_file(char *url, char *outpath) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outpath, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 1;
}
