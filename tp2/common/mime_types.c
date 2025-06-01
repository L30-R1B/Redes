

#include "mime_types.h"
#include <string.h>
#include <stdio.h> 


typedef struct {
    const char *extensao;
    const char *tipo_mime;
} MimeMap;


static MimeMap mime_map[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".pdf", "application/pdf"},
    {".txt", "text/plain"},
    
    {NULL, NULL} 
};

const char* obter_tipo_mime(const char *nome_arquivo) {
    const char *ponto = strrchr(nome_arquivo, '.'); 
    if (ponto) {
        for (int i = 0; mime_map[i].extensao != NULL; ++i) {
            if (strcmp(ponto, mime_map[i].extensao) == 0) {
                return mime_map[i].tipo_mime;
            }
        }
    }
    return "application/octet-stream"; 
}
