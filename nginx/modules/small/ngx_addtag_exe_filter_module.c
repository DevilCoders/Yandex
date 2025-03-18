
/*
 * Copyright (C) Alexander Yashkin <a-v-y@yandex-team.ru>
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_md5.h>

typedef unsigned short uint16;

#define TRUE 1
#define FALSE 0

#ifndef likely
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif
#endif

// Constants

// Tag signature magic bytes
const u_char  kTagMagicBytes[] = "Yact";

// Names of params used from uri query
const u_char  kHashParamName[] = "hash";
const u_char  kDownloadDateParamName[] = "download_date";
// Key used for calculating hash values
const u_char  kSecretKey[] = "u1zAw9Nga0Iu87LRz1XQpN0oAKPRx83Q";

// Max size of tag buffer
const ngx_uint_t  kMaxTagBufferLen = 2048;
// Size of MD5 hash
const ngx_uint_t  kMd5HashLen = 16;

// Offset to PE magic bytes
const ngx_uint_t kPEMagicBytesOffset = 0;
// Size of PE magic bytes
const ngx_uint_t kPEMagicBytesSize = 2;
// MZ - first two bytes of PE module
const uint16_t     kPeMagicBytes = 0x5A4D;
// Offset of PE header in module
const ngx_uint_t kPEHeaderOffset = 60;
// Size of PE header offset
const ngx_uint_t kPEHeaderOffsetSize = 4;
// Offset of certificate directory address relative to PE header
const ngx_uint_t kCertDirAddressOffset = 152;
// Offset of  ASN 1 Signature relative to certificate directory start
const ngx_uint_t kAsn1SigOffset = 8;
// first two bytes of ASN 1 signature
const uint16_t    kAsn1MagicBytes = 0x8230;


// Structures

typedef struct {
  off_t        offset;
  off_t        file_len;

  // Temporary buffer, to keep values content
  // while reading from memory chunks
  u_char       read_buf[4];

  // Buffer with tag content to insert in PE module
  ngx_buf_t*   tag_buffer;

  // Important offsets in PE file that we need to know, to write tag
  uint32_t     pe_header_offset;
  uint32_t     cert_dir_addr;
  uint32_t     tag_foffset;

  // Offsets to params
  u_char      *validated_params_start;
  uint32_t    validated_params_len;
  u_char      *download_date_start;
  uint32_t    download_date_len;
  ngx_uint_t  type;

  // Flags indicating correctness of offsets
  unsigned     pe_magic_valid:1;
  unsigned     pe_header_offset_valid:1;
  unsigned     cert_dir_addr_valid:1;
  unsigned     tag_foffset_valid:1;
  unsigned     validated_params_valid:1;
  unsigned     download_date_valid:1;
} ngx_addtag_exe_filter_ctx_t;

typedef struct {
  ngx_uint_t          type;
} ngx_addtag_exe_filter_conf_t;

static ngx_uint_t ngx_http_addtag_parse_uri(ngx_http_request_t *r,
  ngx_addtag_exe_filter_ctx_t *ctx);

static ngx_int_t ngx_addtag_exe_filter_init(ngx_conf_t *cf);

static void * ngx_addtag_exe_filter_create_conf(ngx_conf_t *cf);
static char * ngx_addtag_exe_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child);

#define NGX_ADDTAG_FILTER_OFF      0
#define NGX_ADDTAG_FILTER_CACHED   1
#define NGX_ADDTAG_FILTER_ON       2
#define NGX_ADDTAG_FILTER_VALIDATE 3

static ngx_conf_enum_t  ngx_addtag_exe_filter_types[] = {
  { ngx_string("off"), NGX_ADDTAG_FILTER_OFF},
#if (NGX_HTTP_CACHE)
  { ngx_string("cached"), NGX_ADDTAG_FILTER_CACHED},
#endif
  { ngx_string("on"), NGX_ADDTAG_FILTER_ON},
  { ngx_string("validate"), NGX_ADDTAG_FILTER_VALIDATE},
  { ngx_null_string, 0 }
};

static ngx_command_t  ngx_addtag_exe_filter_commands[] = {
  { ngx_string("addtag_filter"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_enum_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_addtag_exe_filter_conf_t, type),
    &ngx_addtag_exe_filter_types},

    ngx_null_command
};

static ngx_http_module_t  ngx_addtag_exe_filter_module_ctx = {
  NULL,                                  /* preconfiguration */
  ngx_addtag_exe_filter_init,              /* postconfiguration */

  NULL,                                  /* create main configuration */
  NULL,                                  /* init main configuration */

  NULL,                                  /* create server configuration */
  NULL,                                  /* merge server configuration */

  ngx_addtag_exe_filter_create_conf,       /* create location configuration */
  ngx_addtag_exe_filter_merge_conf         /* merge location configuration */
};


ngx_module_t  ngx_addtag_exe_filter_module = {
  NGX_MODULE_V1,
  &ngx_addtag_exe_filter_module_ctx,       /* module context */
  ngx_addtag_exe_filter_commands,          /* module directives */
  NGX_HTTP_MODULE,                       /* module type */
  NULL,                                  /* init master */
  NULL,                                  /* init module */
  NULL,                                  /* init process */
  NULL,                                  /* init thread */
  NULL,                                  /* exit thread */
  NULL,                                  /* exit process */
  NULL,                                  /* exit master */
  NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static uint32_t
ngx_hexbydword(const uint8_t *line)
{
    uint8_t     c, ch, n = 4, m, val;
    uint32_t  value;
    line += 7;

    for (value = 0; n--; ) {
        value <<= 8;
        val = 0;
        for (m = 2; m--; line--) {
            ch = *line;
            if (ch >= '0' && ch <= '9') {
                val += m == 0 ? (ch - '0') << 4 : (ch - '0');
                continue;
            }

            c = (uint8_t) (ch | 0x20);

            if (c >= 'a' && c <= 'f') {
                val += m == 0 ? (c - 'a' + 10)<<4 : (c - 'a' + 10);
                continue;
            }

            return 0;
        }
        value += val;
    }
    return value;
}



static ngx_int_t
ngx_http_addtag_header_filter(ngx_http_request_t *r)
{
  ngx_addtag_exe_filter_ctx_t  *ctx;
  ngx_addtag_exe_filter_conf_t *ffcf;

  ffcf = ngx_http_get_module_loc_conf(r, ngx_addtag_exe_filter_module);

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "http addtag filter header filter: type: %d", ffcf->type);

  switch(ffcf->type) {
      case NGX_ADDTAG_FILTER_ON:
      case NGX_ADDTAG_FILTER_VALIDATE:
          break;

      case NGX_ADDTAG_FILTER_CACHED:
          if (r->upstream && (r->upstream->cache_status == NGX_HTTP_CACHE_HIT)) {
              break;
          } // fallthrough

      default: /* NGX_ADDTAG_FILTER_OFF */
          return ngx_http_next_header_filter(r);
  }

  if (r->http_version < NGX_HTTP_VERSION_10
      || r->headers_out.status != NGX_HTTP_OK
      || r != r->main
      || r->headers_out.content_length_n == -1) {
        return ngx_http_next_header_filter(r);
    }

  ctx = ngx_pcalloc(r->pool, sizeof(ngx_addtag_exe_filter_ctx_t));
  if (ctx == NULL) {
      return NGX_ERROR;
  }

  ctx->type = ffcf->type;

  switch (ngx_http_addtag_parse_uri(r, ctx)) {

  case NGX_OK: {
      ngx_http_set_ctx(r, ctx, ngx_addtag_exe_filter_module);

      r->headers_out.status = NGX_HTTP_OK;
      if (ffcf->type == NGX_ADDTAG_FILTER_ON) {
          ctx->file_len = r->headers_out.content_length_n;

          // Need body of response in memory to parse module content
          r->filter_need_in_memory = 1;
      }
      break;
  }
  case NGX_ERROR: {
      return NGX_ERROR;
    }
  case NGX_HTTP_BAD_REQUEST: {
      return NGX_HTTP_BAD_REQUEST; // Uri validation failed
    }

  default: /* NGX_DECLINED */
      break;
  }

  return ngx_http_next_header_filter(r);
}

static ngx_buf_t*
ngx_http_addtag_create_tag_buf(ngx_http_request_t *r, ngx_str_t buf_content) {
  // 1. Calculate buffer size
  // The format of the tag buffer is:
  // 000000-000003: 4-byte magic (big-endian)
  // 000004-000005: unsigned 16-bit int string length (big-endian)
  // 000006-??????: ASCII string
  const ngx_uint_t kTagMagicBytesLen = sizeof(kTagMagicBytes) - 1;
  ngx_uint_t tag_string_len = buf_content.len;
  ngx_uint_t tag_header_len = kTagMagicBytesLen + 2;
  ngx_uint_t unpadded_tag_buffer_len = tag_string_len + tag_header_len;
  // The tag buffer should be padded to multiples of 8, otherwise it will
  // break the signature of the executable file.
  ngx_uint_t padded_tag_buffer_length = (unpadded_tag_buffer_len + 15) & (-8);
  // Limit max len of tag
  if (padded_tag_buffer_length > kMaxTagBufferLen) {
    tag_string_len -= (padded_tag_buffer_length - kMaxTagBufferLen);
    padded_tag_buffer_length = kMaxTagBufferLen;
  }

  // 2. Build tag buffer
  ngx_buf_t* tag_buffer = ngx_create_temp_buf(r->pool, padded_tag_buffer_length);
  if (tag_buffer == NULL) return tag_buffer;
  // Zero buffer content
  ngx_memzero(tag_buffer->pos, padded_tag_buffer_length);

  tag_buffer->last = tag_buffer->start + padded_tag_buffer_length;
  tag_buffer->memory = 1;
  tag_buffer->sync = 1;

  // Copy magic bytes at start of buffer
  ngx_memcpy(tag_buffer->pos, kTagMagicBytes, kTagMagicBytesLen);
  // Write string len after magic bytes
  tag_buffer->pos[kTagMagicBytesLen] = (u_char)((tag_string_len & 0xff00) >> 8);
  tag_buffer->pos[kTagMagicBytesLen+1] = (u_char)(tag_string_len & 0xff);

  // Copy buffer content string to tag buffer
  ngx_memcpy(tag_buffer->pos + tag_header_len, buf_content.data, tag_string_len);

  return tag_buffer;
}

ngx_int_t
ngx_http_arg_get_name_value(ngx_http_request_t *r,
                            u_char *name,
                            size_t len,
                            ngx_str_t *name_found,
                            ngx_str_t *value) {
  ngx_int_t status = ngx_http_arg(r, name, len, value);
  if (status == NGX_OK) {
    name_found->data = value->data - 1 - len; // Skip back to name of parameter
    name_found->len = len;
  }
  return status;
}

static ngx_uint_t
ngx_http_addtag_validate_args(ngx_http_request_t *r,
                              ngx_addtag_exe_filter_ctx_t *ctx) {
  ngx_str_t   hash_name, hash_value, ddate_value;
  ngx_md5_t   md5;
  u_char      hash_calculated[kMd5HashLen];
  ngx_uint_t  i, n;
  u_char      *validated_params_end;
  time_t      download_date;

  // Find "hash" parameter with md5 hash value
  if (ngx_http_arg_get_name_value(r,
                   (u_char *)kHashParamName,
                   sizeof(kHashParamName) - 1,
                   &hash_name,
                   &hash_value) != NGX_OK) {
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
      "http addtag filter validate args uri hash argument not found");
    return NGX_ERROR;
  }

  // Check hash length
  // every byte of hash is represented by 2 symbols in hex encoding
  if (hash_value.len != kMd5HashLen * 2) {
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
      "http addtag filter validate args uri hash argument invalid length");
    return NGX_ERROR;
  }

  // All arguments from start of query to "hash=" must be validated, calculate
  // hash on them and compare to value in request
  ctx->validated_params_start = r->args.data;
  // Set validated params end to start of "&hash="
  validated_params_end = hash_name.data;
  if (likely(hash_name.data != ctx->validated_params_start)) {
    validated_params_end -= 1; // Skip previous '&' character
    ctx->validated_params_len = validated_params_end - ctx->validated_params_start;
  } else {
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
      "http addtag filter validate args md5 hash present, but no arguments to hash");
    return NGX_ERROR;
  }

  // Calculate md5 hash of secret key and string before "hash=" parameter
  // and compare to value of hash param
  ngx_md5_init(&md5);
  ngx_md5_update(&md5, ctx->validated_params_start, ctx->validated_params_len);
  ngx_md5_update(&md5, kSecretKey, sizeof(kSecretKey) - 1);
  ngx_md5_final(hash_calculated, &md5);

  // Compare calculated hash value with value extracted from GET params
  for (i = 0; i < kMd5HashLen/4; i++) {
    n = ngx_hexbydword(&hash_value.data[8 * i]);
    if (n != ((uint32_t *)hash_calculated)[i]) {
      ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
        "http addtag filter validate args md5 hash not valid");
      return NGX_ERROR;
//      break;  // DEBUG ONLY
    }
  }
  ctx->validated_params_valid = 1;

  // Find "download_date" param - it is optional
  if (ngx_http_arg(r,
                   (u_char *)kDownloadDateParamName,
                   sizeof(kDownloadDateParamName) - 1,
                   &ddate_value) == NGX_OK) {
    // Check date validity, parse date from string
    download_date = ngx_atotm(ddate_value.data, ddate_value.len);
    if (download_date == NGX_ERROR) {
      ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
        "http addtag filter validate args download_date invalid format");
      return NGX_ERROR;
    }
    ctx->download_date_start = ddate_value.data;
    ctx->download_date_len = ddate_value.len;
    ctx->download_date_valid = 1;
  }
  return NGX_OK;
}

static ngx_uint_t
ngx_http_addtag_init_tag_buf_content(ngx_http_request_t *r,
                                     ngx_str_t* tag_content,
                                     ngx_addtag_exe_filter_ctx_t *ctx) {
  ngx_int_t tag_content_len;
  u_char *tag_data;
  // Create string which contains hashed arguments and download_date to
  // write it inside browser distrib
  tag_content_len = ctx->validated_params_len + ctx->download_date_len;
  if (ctx->download_date_valid !=0) {
    tag_content_len += sizeof(kDownloadDateParamName) + 1; // Add place for '&' separator symbol and '='
    // sizeof(kDownloadDateParamName) = strlen(kDownloadDateParamName) + 1
  }
  tag_content->data = ngx_pcalloc(r->pool, tag_content_len);
  if (tag_content->data == NULL) {
    return NGX_ERROR;
  }

  // Concatenate tag content to result string
  ngx_memcpy(tag_content->data, ctx->validated_params_start, ctx->validated_params_len);
  if (ctx->download_date_valid !=0) {
    tag_data = tag_content->data + ctx->validated_params_len;
    tag_data[0] = '&'; tag_data++;// Insert separator symbol
    ngx_memcpy(tag_data, kDownloadDateParamName, sizeof(kDownloadDateParamName)-1);
    tag_data += sizeof(kDownloadDateParamName)-1;
    tag_data[0] = '='; tag_data++;// Insert equality symbol
    ngx_memcpy(tag_data, ctx->download_date_start, ctx->download_date_len);
  }

  tag_content->len = tag_content_len;
  return NGX_OK;
}


// Parse uri and create buffer with tag content to append at PE file
static ngx_uint_t
ngx_http_addtag_parse_uri(ngx_http_request_t *r,
                          ngx_addtag_exe_filter_ctx_t *ctx) {
  ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "http addtag filter parse uri \"%V?%V\"", &r->uri, &r->args);

  if (r->args.len) {
    // If additional arguments present, they must contain md5 hash
    // which we must validate, before writing these arguments to distributive
    if (ngx_http_addtag_validate_args(r, ctx) == NGX_OK) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter parse uri validate ok");

      if (ctx->type == NGX_ADDTAG_FILTER_VALIDATE) {
          return NGX_OK;
      }
      // Init string to write inside distributive
      ngx_str_t tag_buf_content;
      if (ngx_http_addtag_init_tag_buf_content(r, &tag_buf_content, ctx) != NGX_OK) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter parse uri failed to init tag buffer");
        return NGX_DECLINED;
      }

      ctx->tag_buffer = ngx_http_addtag_create_tag_buf(r, tag_buf_content);
      if (ctx->tag_buffer == NULL) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter parse uri failed to create tag buffer");
        return NGX_DECLINED;
      }
    } else {
        // URI validation failed, log referer and redirect to correct link
        if (r->headers_in.referer != NULL && r->headers_in.referer->value.data != NULL) {
          ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
            "http addtag filter validation of uri arguments failed (referer = %s)", r->headers_in.referer->value.data);
        } else {
          ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
            "http addtag filter validation of uri arguments failed (no referer)");
        }
        return NGX_HTTP_BAD_REQUEST;
    }
  }
  ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter parse uri success");
  return NGX_OK; // Allow download requests with no parameters
}


static ngx_uint_t
ngx_http_addtag_ranges_intersects(off_t start1,
                                  off_t last1,
                                  off_t start2,
                                  off_t last2) {
  if ( last1 <= start2 || start1 >= last2 ) return FALSE;
    return TRUE;
}

static ngx_uint_t
ngx_http_addtag_collect_byte_buffer_from_chain(
                                      ngx_http_request_t *r,
                                      ngx_chain_t* cl,
                                      off_t  chain_link_start_foffset,
                                      off_t  buffer_start_foffset,
                                      off_t  buffer_last_foffset,
                                      u_char* out_buffer) {
  ngx_buf_t   *buf = cl->buf;
  off_t       chain_link_last_foffset =
              chain_link_start_foffset + ngx_buf_size(buf);
  off_t i;

  // Extract bytes in range [buffer_start_foffset,buffer_last_foffset] from chain buffer
  if (ngx_http_addtag_ranges_intersects(
          chain_link_start_foffset,
          chain_link_last_foffset,
          buffer_start_foffset,
          buffer_last_foffset)) {
    off_t start_offset = ngx_max(chain_link_start_foffset, buffer_start_foffset);
    off_t end_offset = ngx_min(chain_link_last_foffset, buffer_last_foffset);
    off_t num_bytes = end_offset - start_offset;

    off_t input_buf_pos = start_offset - chain_link_start_foffset;
    off_t ouput_buf_pos = start_offset - buffer_start_foffset;

    for (i = 0; i< num_bytes; i++) {
      out_buffer[ouput_buf_pos++] = buf->pos[input_buf_pos++];
    }
    if (chain_link_last_foffset >= buffer_last_foffset) return TRUE;
  }
  return FALSE;
}

// Function uses ctx->read_buf as temporary buffer
static ngx_uint_t
extract_uint16_from_chain(ngx_http_request_t *r,
                          ngx_addtag_exe_filter_ctx_t* ctx,
                          ngx_chain_t* cl,
                          off_t  cl_foffset,
                          off_t  value_foffset,
                          uint16_t* value) {
  //unsigned char i;
  ngx_uint_t extract_result = ngx_http_addtag_collect_byte_buffer_from_chain(
                  r,
                  cl,
                  cl_foffset,
                  value_foffset,
                  value_foffset + sizeof(uint16_t),
                  ctx->read_buf);
  if (extract_result) {
    memcpy((void *)value, ctx->read_buf, sizeof(uint16_t));
  }
  return extract_result;
}

// Function uses ctx->read_buf as temporary buffer
static ngx_uint_t
extract_uint32_from_chain(ngx_http_request_t *r,
                          ngx_addtag_exe_filter_ctx_t* ctx,
                          ngx_chain_t* cl,
                          off_t  cl_foffset,
                          off_t  value_offset,
                          uint32_t* value) {
  //unsigned char i;
  ngx_uint_t extract_result = ngx_http_addtag_collect_byte_buffer_from_chain(
                  r,
                  cl,
                  cl_foffset,
                  value_offset,
                  value_offset + sizeof(uint32_t),
                  ctx->read_buf);
  if (extract_result) {
    memcpy((void *)value, ctx->read_buf, sizeof(uint32_t));
  }
  return extract_result;
}

// Function parses chunks of PE file and extracts necessary offsets to
// insert tag at the end of digital signature
static void
process_chain_link_parse_pe_module(ngx_http_request_t *r,
                                   ngx_addtag_exe_filter_ctx_t  *ctx,
                                   ngx_chain_t *cl) {

  // start file offset of chain link block
  off_t start_foffset = ctx->offset;

  // Read and check PE signature bytes
  uint16_t pe_sig;
  ngx_uint_t extract_result = extract_uint16_from_chain(r, ctx, cl, start_foffset,
                                                  kPEMagicBytesOffset,
                                                  &pe_sig);
  if (extract_result) {
    if (pe_sig != kPeMagicBytes) {
        // Skip processing - not a PE file
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
          "http addtag filter body buf: PE sig invalid = 0x%Xd", (uint32_t)pe_sig);
        ctx->pe_magic_valid = FALSE;
        return;
    } else {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http addtag filter body buf: PE sig valid");
        ctx->pe_magic_valid = TRUE;
      }
  }

  if (ctx->pe_magic_valid) {
    // Extract PE header offset
    uint32_t pe_header_offset;
    extract_result = extract_uint32_from_chain(r, ctx, cl, start_foffset,
                                                    kPEHeaderOffset,
                                                    &pe_header_offset);
    if (extract_result && pe_header_offset != 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter body buf: PE header offset = 0x%Xd", pe_header_offset);
      ctx->pe_header_offset = pe_header_offset;
      ctx->pe_header_offset_valid = TRUE;
    }
  }

  // Read certificates data directory info
  if (ctx->pe_header_offset_valid) {
    uint32_t cert_dir_addr_offset = ctx->pe_header_offset + kCertDirAddressOffset;
    uint32_t cert_dir_len_offset = ctx->pe_header_offset + kCertDirAddressOffset + 4;

    uint32_t cert_dir_addr;
    extract_result = extract_uint32_from_chain(r, ctx, cl, start_foffset,
                                                        cert_dir_addr_offset,
                                                        &cert_dir_addr);
    if (extract_result && cert_dir_addr != 0) {
      ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter body buf: certificate directory offset = 0x%Xd", cert_dir_addr);
      ctx->cert_dir_addr = cert_dir_addr;
      ctx->cert_dir_addr_valid = TRUE;
    }

    uint32_t cert_dir_len;
    extract_result = extract_uint32_from_chain(r, ctx, cl, start_foffset,
                                               cert_dir_len_offset,
                                               &cert_dir_len);
    if (extract_result) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http addtag filter body buf: certificate directory length = %d", cert_dir_len);
        if(cert_dir_len == 0) { // Zero size of certificates directory
          ctx->cert_dir_addr_valid = FALSE;
        }
    }
  }

  if (ctx->cert_dir_addr_valid) {
    // Certificate directory is container for signatures
    // (only 1 is used usually)
    // Get header of first signature, and calculate tag offset
    uint32_t first_signature_offset = ctx->cert_dir_addr + kAsn1SigOffset;

    // Header contains magic bytes and size
    uint32_t asn1_sig_header;
    extract_result = extract_uint32_from_chain(r, ctx, cl, start_foffset,
                                                        first_signature_offset,
                                                        &asn1_sig_header);
    if (extract_result) {
      ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
          "http addtag filter body buf: asn1 signature header = 0x%Xd", asn1_sig_header);
      // Check magic bytes at asn1 start
      // fix strict aliasing warning
      uint16_t asn_magic;
      memcpy(&asn_magic, &asn1_sig_header, 2);
#ifndef NGX_HAVE_LITTLE_ENDIAN
#if (__builtint_swap16)
      asn_magic = __builtint_swap16(asn_magic);
#else
      asn_magic = ((asn_magic & 0xFF) << 8) | (asn_magic >> 8);
#endif
#endif
      if (asn_magic == kAsn1MagicBytes) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "http addtag filter body buf: asn1 signature valid magic");

        // Extract asn1 certificate size and calculate tag offset from it
        uint16_t asn1_size = (uint16_t)(asn1_sig_header >> 16);
        // Asn1 signature size is in MSB format, so change bytes order to LSB
#ifdef NGX_HAVE_LITTLE_ENDIAN
#if (__builtint_swap16)
        asn1_size = __builtint_swap16(asn1_size);
#else
        asn1_size = ((asn1_size & 0xFF) << 8) | (asn1_size >> 8);
#endif
#endif
        // Windows pads the certificate directory to align at a 8-byte boundary.
        // This piece of code it trying to replicate the logic that is used to
        // calculate the padding to be added to the certificate. It returns
        // the entire length of the certificate directory from the windows
        // certificate directory start to the end of padding.
        // The windows certificate directory has the following structure
        // <WIN_CERTIFICATE><Certificate><Padding>.
        // WIN_CERTIFICATE is the windows certificate directory structure.
        // <Certificate> has the following format:
        // <Magic(2 bytes)><Cert length(2 bytes)><Certificate Data>
        // Note that the "Cert length" does not include the magic bytes or
        // the length.
        //
        // Hence the total length of the certificate is:
        // cert_length + "WIN_CERTIFICATE header size" + magic + length
        // + padding = (cert length + 8 + 2 + 2 + 7) & (0-8)
        uint32_t cert_total_size = ((uint32_t)asn1_size + 8 + 2 + 2 + 7) & 0xffffff8;
        ctx->tag_foffset = ctx->cert_dir_addr + cert_total_size;
        // Check if file is big enough for tag
        off_t tag_last_foffset = ctx->tag_foffset + ngx_buf_size(ctx->tag_buffer);
        if (tag_last_foffset <= ctx->file_len) {
          ctx->tag_foffset_valid = TRUE;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "http addtag filter body buf: cert_total_size = 0x%Xd, tag offset calculated = 0x%Xd", cert_total_size, (uint32_t)ctx->tag_foffset);
      } else {
          ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
              "http addtag filter body buf: asn1 signature invalid magic = 0x%Xd", (uint32_t)asn_magic);
      }
    }
  }
}

static ngx_chain_t*
process_chain_link_insert_tag_buffer(ngx_http_request_t *r,
                                     ngx_chain_t *cl,
                                     off_t cl_start_foffset,
                                     off_t cl_last_foffset,
                                     off_t tag_foffset,
                                     off_t tag_last_foffset,
                                     ngx_buf_t* tag_buffer) {
  if (ngx_http_addtag_ranges_intersects(
          cl_start_foffset,
          cl_last_foffset,
          tag_foffset,
          tag_last_foffset)) {
    ngx_buf_t*  buf = cl->buf;

    ngx_log_debug6(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http addtag filter body chain offset: %O-%O, chain mem: %O-%O, tag offset: %O-%O",
                    cl_start_foffset,
                    cl_last_foffset,
                    buf->pos,
                    buf->last,
                    tag_foffset,
                    tag_last_foffset);

    // 1. Chain link is inside tag buffer - skip link content
    if (cl_start_foffset >= tag_foffset && cl_last_foffset <= tag_last_foffset) {
      ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                     "http addtag filter body chain link skipped: %O-%O",
                     cl_start_foffset,
                     cl_last_foffset);

      if (buf->in_file) {
        buf->file_pos = buf->file_last;
      }

      buf->pos = buf->last;
      buf->sync = 1;
      return cl;
    }

    // 2. Chain link starts before tag buffer
    if (cl_start_foffset < tag_foffset) {
      // Create chain link with tag content
      ngx_chain_t* tag_cl = ngx_alloc_chain_link(r->pool);

      tag_cl->buf = ngx_alloc_buf(r->pool);
      *tag_cl->buf = *tag_buffer;

      ngx_chain_t *last_cl;

      // 2.1 Chain link ends after tag buffer, split chain link on two
      if (cl_last_foffset > tag_last_foffset) {
        // Split chain link, create new link after tag buffer
        off_t new_chunk_rel_offset = tag_last_foffset - cl_start_foffset;
        ngx_chain_t *new_cl = ngx_alloc_chain_link(r->pool);
        new_cl->buf = ngx_calloc_buf(r->pool);

        // Copy buffer info from original chain link
        *new_cl->buf = *buf;

        if (ngx_buf_in_memory(buf)) {
          new_cl->buf->pos = buf->pos + new_chunk_rel_offset;
        }
        new_cl->buf->sync = 1;
        if (buf->in_file) {
          new_cl->buf->file_pos = buf->file_pos + new_chunk_rel_offset;
        }
        // Link new chain link to old one
        new_cl->next = cl->next;
        cl->next = new_cl;
        last_cl = new_cl;

        ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http addtag filter body chain link after tag created: %O-%O, foffsets %O-%O",
                        (off_t)new_cl->buf->pos,
                        (off_t)new_cl->buf->last,
                        (off_t)new_cl->buf->file_pos,
                        (off_t)new_cl->buf->file_last);

      } else {
        last_cl = tag_cl;
      }

      // Insert tag chain link in list
      tag_cl->next = cl->next;
      cl->next = tag_cl;

      // Update offsets of original chain link so it will end at tag start
      if (cl->buf->in_file) {
        cl->buf->file_last = cl->buf->file_pos + (tag_foffset - cl_start_foffset);
      }
      if (ngx_buf_in_memory(cl->buf)) {
        cl->buf->last = cl->buf->pos + (tag_foffset - cl_start_foffset);
      }

      ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                     "http addtag filter body chain link before tag updated: %O-%O, foffsets %O-%O",
                      (off_t)cl->buf->pos,
                      (off_t)cl->buf->last,
                      (off_t)cl->buf->file_pos,
                      (off_t)cl->buf->file_last);

      last_cl->buf->last_buf = cl->buf->last_buf;
      last_cl->buf->last_in_chain = cl->buf->last_in_chain;
      cl->buf->last_buf = 0;
      cl->buf->last_in_chain = 0;

      return last_cl;
    }
    // 3. Chain link starts inside tag buffer, simply update offsets
    else {
      if (buf->in_file) {
        buf->file_pos = buf->file_pos + (tag_last_foffset - cl_start_foffset);
      }
      if (ngx_buf_in_memory(buf)) {
        buf->pos = buf->pos + (tag_last_foffset - cl_start_foffset);
      }
      return cl;
    }
  }
  return cl;
}

static ngx_int_t
ngx_http_addtag_body_filter(ngx_http_request_t *r, ngx_chain_t *in) {
  ngx_addtag_exe_filter_ctx_t  *ctx;
  off_t                       cl_start_foffset, cl_last_foffset;
  ngx_buf_t                   *buf;
  ngx_chain_t                 *cl;
  ngx_addtag_exe_filter_conf_t *ffcf;

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                 "http addtag filter body filter");

  if (in == NULL) {
      return ngx_http_next_body_filter(r, in);
  }

  ffcf = ngx_http_get_module_loc_conf(r, ngx_addtag_exe_filter_module);
  if (ffcf->type == NGX_ADDTAG_FILTER_OFF || ffcf->type == NGX_ADDTAG_FILTER_VALIDATE) {
      return ngx_http_next_body_filter(r, in);
  }

  ctx = ngx_http_get_module_ctx(r, ngx_addtag_exe_filter_module);

  if (ctx == NULL || ctx->tag_buffer == NULL) {
      return ngx_http_next_body_filter(r, in);
  }

  for (cl = in; cl; cl = cl->next) {
    buf = cl->buf;

    cl_start_foffset = ctx->offset;
    cl_last_foffset = ctx->offset + ngx_buf_size(buf);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http addtag filter body buf: %O-%O", cl_start_foffset, cl_last_foffset);

    // Extract offsets and fields of PE structures
    // to calculate offset of tag buffer
    process_chain_link_parse_pe_module(r, ctx, cl);

    // Update current file offset
    ctx->offset += ngx_buf_size(buf);

    // Insert tag buffer in chain, if tag buffer offset is already extracted
    if (ctx->tag_foffset_valid && (ctx->tag_buffer != NULL)) {
      off_t tag_last_foffset = ctx->tag_foffset + ngx_buf_size(ctx->tag_buffer);
      // If chunk intersects with tag buffer - insert tag buffer and correct chunk sizes
      if (ngx_http_addtag_ranges_intersects(
            cl_start_foffset,
            cl_last_foffset,
            ctx->tag_foffset,
            tag_last_foffset)) {
        cl = process_chain_link_insert_tag_buffer(
              r,
              cl,
              cl_start_foffset,
              cl_last_foffset,
              ctx->tag_foffset,
              tag_last_foffset,
              ctx->tag_buffer);
      }
    }
  }
  return ngx_http_next_body_filter(r, in);
}

static ngx_int_t
ngx_addtag_exe_filter_init(ngx_conf_t *cf) {
  ngx_http_next_header_filter = ngx_http_top_header_filter;
  ngx_http_top_header_filter = ngx_http_addtag_header_filter;

  ngx_http_next_body_filter = ngx_http_top_body_filter;
  ngx_http_top_body_filter = ngx_http_addtag_body_filter;

  return NGX_OK;
}

static void *
ngx_addtag_exe_filter_create_conf(ngx_conf_t *cf) {
  ngx_addtag_exe_filter_conf_t  *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_addtag_exe_filter_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->type = NGX_CONF_UNSET_UINT;
  return conf;
}

static char *
ngx_addtag_exe_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_addtag_exe_filter_conf_t *prev = parent;
    ngx_addtag_exe_filter_conf_t *conf = child;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, cf->log, 0,
                   "http addtag filter merge conf, parent = %d, child = %d", (ngx_uint_t)prev->type, (ngx_uint_t)conf->type);

    ngx_conf_merge_uint_value(conf->type, prev->type, NGX_ADDTAG_FILTER_OFF);
    return NGX_CONF_OK;
}
