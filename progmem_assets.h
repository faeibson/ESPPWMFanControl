
#ifndef _PROGMEM_ASSETS_H_
#define _PROGMEM_ASSETS_H_

#include "progmem_assets_css.h"
#include "progmem_assets_fonts.h"
#include "progmem_assets_images.h"
#include "progmem_assets_javascript.h"

namespace progmem_assets_strings {
    static const char location[] PROGMEM = "Location";

    // mime types
    static const char mime_application_javascript[] PROGMEM = "application/javascript";
    static const char mime_application_json[] PROGMEM = "application/json";
    static const char mime_application_x_gzip[] PROGMEM = "application/x-gzip";
    static const char mime_application_x_pdf[] PROGMEM = "application/x-pdf";
    static const char mime_application_x_zip[] PROGMEM = "application/x-zip";
    static const char mime_application_octet_stream[] PROGMEM = "application/octet-stream";
    static const char mime_image_gif[] PROGMEM = "image/gif";
    static const char mime_image_jpeg[] PROGMEM = "image/jpeg";
    static const char mime_image_png[] PROGMEM = "image/png";
    static const char mime_image_x_icon[] PROGMEM = "image/x-icon";
    static const char mime_text_css[] PROGMEM = "text/css";
    static const char mime_text_html[] PROGMEM = "text/html";
    static const char mime_text_plain[] PROGMEM = "text/plain";
    static const char mime_text_xml[] PROGMEM = "text/xml";
};

#endif
