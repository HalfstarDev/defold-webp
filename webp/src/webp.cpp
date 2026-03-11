#define EXTENSION_NAME Webp
#define LIB_NAME "Webp"
#define MODULE_NAME "webp"

#include <dmsdk/sdk.h>
#include "decode.h"
#include "encode.h"

static bool IsWebPData(const uint8_t* data, size_t size)
{
    return size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0;
}

static bool LoadFile(lua_State* L, const char* path, uint8_t** outData, size_t* outSize)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* buf = (uint8_t*)malloc((size_t)sz);
    if (!buf)
    {
        fclose(f);
        return false;
    }

    fread(buf, 1, (size_t)sz, f);
    fclose(f);

    *outData = buf;
    *outSize = (size_t)sz;
    return true;
}

// Load data from argument, either as WebP file data, or as path to WebP file
static bool LoadData(lua_State* L, int idx, const uint8_t** data, size_t* size, uint8_t** owned)
{
    size_t argLen = 0;
    const char* arg = luaL_checklstring(L, idx, &argLen);
    *owned = nullptr;

    if (IsWebPData((const uint8_t*)arg, argLen))
    {
        *data = (const uint8_t*)arg;
        *size = argLen;
        return true;
    }
    
    if (!LoadFile(L, arg, owned, size))
    {
        luaL_error(L, "webp: could not load image");
        return false; 
    }

    *data = *owned;
    return true;
}

static void ReadBoolField(lua_State* L, int tbl, const char* key, int* out)
{
    lua_getfield(L, tbl, key);
    if (!lua_isnil(L, -1))
    {
        *out = lua_toboolean(L, -1) ? 1 : 0;
    }
    lua_pop(L, 1);
}

static void ReadIntField(lua_State* L, int tbl, const char* key, int* out)
{
    lua_getfield(L, tbl, key);
    if (lua_isnumber(L, -1))
    {
        *out = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
}

static void ReadFloatField(lua_State* L, int tbl, const char* key, float* out)
{
    lua_getfield(L, tbl, key);
    if (lua_isnumber(L, -1))
    {
        *out = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}

// Fills WebPDecoderOptions from an optional Lua table.
static void ParseDecoderOptions(lua_State* L, int tbl, WebPDecoderOptions* opts)
{
    // Flip image by default, so image is upright in Defold
    opts->flip = 1;

    if (!lua_istable(L, tbl))
    {
        return;
    }

    // https://developers.google.com/speed/webp/docs/api#advanced_decoding_api
    ReadBoolField(L, tbl, "bypass_filtering",        &opts->bypass_filtering);
    ReadBoolField(L, tbl, "no_fancy_upsampling",     &opts->no_fancy_upsampling);
    ReadBoolField(L, tbl, "use_cropping",            &opts->use_cropping);
    ReadIntField (L, tbl, "crop_left",               &opts->crop_left);
    ReadIntField (L, tbl, "crop_top",                &opts->crop_top);
    ReadIntField (L, tbl, "crop_width",              &opts->crop_width);
    ReadIntField (L, tbl, "crop_height",             &opts->crop_height);
    ReadBoolField(L, tbl, "use_scaling",             &opts->use_scaling);
    ReadIntField (L, tbl, "scaled_width",            &opts->scaled_width);
    ReadIntField (L, tbl, "scaled_height",           &opts->scaled_height);
    ReadBoolField(L, tbl, "use_threads",             &opts->use_threads);
    ReadIntField (L, tbl, "dithering_strength",      &opts->dithering_strength);
    ReadIntField (L, tbl, "alpha_dithering_strength",&opts->alpha_dithering_strength);

    // Invert flip, as Defold buffers expect the image in the other direction vertically
    lua_getfield(L, tbl, "flip");
    if (!lua_isnil(L, -1))
    {
        opts->flip = lua_toboolean(L, -1) ? 0 : 1;
    }
    lua_pop(L, 1);
}

// Fills WebPConfig from an optional Lua table.
static void ParseEncoderConfig(lua_State* L, int tbl, WebPConfig* config)
{
    if (!lua_istable(L, tbl))
    {
        return;
    }

    // https://developers.google.com/speed/webp/docs/api#advanced_encoding_api
    ReadBoolField (L, tbl, "lossless",          &config->lossless);
    ReadFloatField(L, tbl, "quality",           &config->quality);
    ReadIntField  (L, tbl, "method",            &config->method);
    ReadIntField  (L, tbl, "image_hint",        (int*)&config->image_hint);
    ReadIntField  (L, tbl, "target_size",       &config->target_size);
    ReadFloatField(L, tbl, "target_PSNR",       &config->target_PSNR);
    ReadIntField  (L, tbl, "segments",          &config->segments);
    ReadIntField  (L, tbl, "sns_strength",      &config->sns_strength);
    ReadIntField  (L, tbl, "filter_strength",   &config->filter_strength);
    ReadIntField  (L, tbl, "filter_sharpness",  &config->filter_sharpness);
    ReadIntField  (L, tbl, "filter_type",       &config->filter_type);
    ReadBoolField (L, tbl, "autofilter",        &config->autofilter);
    ReadIntField  (L, tbl, "alpha_compression", &config->alpha_compression);
    ReadIntField  (L, tbl, "alpha_filtering",   &config->alpha_filtering);
    ReadIntField  (L, tbl, "alpha_quality",     &config->alpha_quality);
    ReadIntField  (L, tbl, "pass",              &config->pass);
    ReadIntField  (L, tbl, "preprocessing",     &config->preprocessing);
    ReadIntField  (L, tbl, "partitions",        &config->partitions);
    ReadIntField  (L, tbl, "partition_limit",   &config->partition_limit);
    ReadBoolField (L, tbl, "emulate_jpeg_size", &config->emulate_jpeg_size);
    ReadBoolField (L, tbl, "thread_level",      &config->thread_level);
    ReadBoolField (L, tbl, "low_memory",        &config->low_memory);
    ReadIntField  (L, tbl, "near_lossless",     &config->near_lossless);
    ReadBoolField (L, tbl, "exact",             &config->exact);
}

static const char* GetWebPError(VP8StatusCode status)
{
    switch (status) {
        case VP8_STATUS_OK:                   return nullptr;
        case VP8_STATUS_OUT_OF_MEMORY:        return "VP8_STATUS_OUT_OF_MEMORY";
        case VP8_STATUS_INVALID_PARAM:        return "VP8_STATUS_INVALID_PARAM";
        case VP8_STATUS_BITSTREAM_ERROR:      return "VP8_STATUS_BITSTREAM_ERROR";
        case VP8_STATUS_UNSUPPORTED_FEATURE:  return "VP8_STATUS_UNSUPPORTED_FEATURE";
        case VP8_STATUS_SUSPENDED:            return "VP8_STATUS_SUSPENDED";
        case VP8_STATUS_USER_ABORT:           return "VP8_STATUS_USER_ABORT";
        case VP8_STATUS_NOT_ENOUGH_DATA:      return "VP8_STATUS_NOT_ENOUGH_DATA";
        default:                              return "VP8 STATUS ERROR";
    }
}

// webp.decode(image [, options])
// image:   string with raw WebP bytes OR a file path
// options: optional Lua table with WebPDecoderOptions as keys
//            https://developers.google.com/speed/webp/docs/api#advanced_decoding_api
// returns: buf, width, height
static int Decode(lua_State* L)
{
    // Get arguments
    
    const uint8_t* data  = nullptr;
    size_t         size  = 0;
    uint8_t*       owned = nullptr;

    if (!LoadData(L, 1, &data, &size, &owned))
    {
        return 0;
    }

    WebPDecoderConfig config;
    WebPInitDecoderConfig(&config);
    ParseDecoderOptions(L, 2, &config.options);
    
    VP8StatusCode status = WebPGetFeatures(data, size, &config.input);
    if (GetWebPError(status))
    {
        if (owned) free(owned);
        return luaL_error(L, "webp: %s", GetWebPError(status));
    }

    const int width  = config.input.width;
    const int height = config.input.height;

    // Create Defold buffer
    
    dmBuffer::StreamDeclaration streams[] = { { dmHashString64("rgba"), dmBuffer::VALUE_TYPE_UINT8, 4 } };
    dmBuffer::HBuffer hBuffer = 0;
    dmBuffer::Result bufResult = dmBuffer::Create((uint32_t)(width * height), streams, 1, &hBuffer);
    if (bufResult != dmBuffer::RESULT_OK)
    {
        return luaL_error(L, "webp: dmBuffer::Create failed (%d)", bufResult);
    }

    // Fill Defold buffer
    
    uint8_t* bufData   = nullptr;
    uint32_t bufCount  = 0;
    uint32_t bufComponents  = 0;
    uint32_t bufStride = 0;
    dmBuffer::GetStream(hBuffer, dmHashString64("rgba"), (void**)&bufData, &bufCount, &bufComponents, &bufStride);

    config.output.colorspace          = MODE_RGBA;
    config.output.u.RGBA.rgba         = bufData;
    config.output.u.RGBA.stride       = width * 4;
    config.output.u.RGBA.size         = (size_t)(width * height * 4);
    config.output.is_external_memory  = 1;

    status = WebPDecode(data, size, &config);
    WebPFreeDecBuffer(&config.output);

    if (GetWebPError(status))
    {
        if (owned) free(owned);
        return luaL_error(L, "webp: %s", GetWebPError(status));
    }

    dmScript::LuaHBuffer luaBuffer = { hBuffer, dmScript::OWNER_LUA };
    dmScript::PushBuffer(L, luaBuffer);
    lua_pushinteger(L, width);
    lua_pushinteger(L, height);

    if (owned) free(owned);

    return 3; // buf, width, height
}

// webp.encode(buffer, width, height [, options])
// buffer:  Defold buffer with a RGBA stream
// width:   buffer image width in pixels
// height:  buffer image height in pixels
// options: optional table with WebPConfig fields (quality, lossless, ...)
//            https://developers.google.com/speed/webp/docs/api#advanced_encoding_api
// returns: string of raw WebP file bytes
static int Encode(lua_State* L)
{
    // Get arguments

    dmScript::LuaHBuffer* luaBuffer = dmScript::CheckBuffer(L, 1);
    if (!luaBuffer)
    {
        return luaL_error(L, "webp: argument 1 must be a buffer");
    }

    int width  = (int)luaL_checkinteger(L, 2);
    int height = (int)luaL_checkinteger(L, 3);

    if (width <= 0 || height <= 0)
    {
        return luaL_error(L, "webp: width and height must be positive integers");
    }

    // Get raw RGBA bytes from buffer stream

    uint8_t* bufData        = nullptr;
    uint32_t bufCount       = 0;
    uint32_t bufComponents  = 0;
    uint32_t bufStride      = 0;
    dmBuffer::Result bufResult = dmBuffer::GetStream(
        luaBuffer->m_Buffer,
        dmHashString64("rgba"),
        (void**)&bufData,
        &bufCount,
        &bufComponents,
        &bufStride
    );

    if (bufResult != dmBuffer::RESULT_OK)
    {
        return luaL_error(L, "webp: failed to get 'rgba' stream (%d)", bufResult);
    }
    if ((int)bufCount < width * height)
    {
        return luaL_error(L, "webp: buffer too small for %dx%d image", width, height);
    }
    if (bufComponents != 4 || bufStride != 4) {
        return luaL_error(L, "webp: buffer must be tightly packed RGBA (stride=4)");
    }

    // Build WebP encoder config

    WebPConfig config;
    if (!WebPConfigInit(&config))
    {
        return luaL_error(L, "webp: WebPConfigInit failed");
    }

    ParseEncoderConfig(L, 4, &config);

    if (!WebPValidateConfig(&config))
    {
        return luaL_error(L, "webp: invalid encoder config");
    }

    // Create WebPPicture

    WebPPicture picture;
    if (!WebPPictureInit(&picture))
    {
        return luaL_error(L, "webp: WebPPictureInit failed");
    }

    picture.use_argb = 1;   // RGBA path; libwebp converts to YUV internally if needed
    picture.width    = width;
    picture.height   = height;

    const int       rowBytes    = width * 4;
    const uint8_t*  lastRow     = bufData + (size_t)(height - 1) * rowBytes;

    if (!WebPPictureImportRGBA(&picture, lastRow, -rowBytes))  // Use rows backwards as Defold buffer is flipped vertically
    {
        WebPPictureFree(&picture);
        return luaL_error(L, "webp: WebPPictureImportRGBA failed");
    }

    // Write to buffer

    WebPMemoryWriter writer;
    WebPMemoryWriterInit(&writer);

    picture.writer     = WebPMemoryWrite;
    picture.custom_ptr = &writer;

    int encodeOk = WebPEncode(&config, &picture);

    WebPEncodingError errorCode = picture.error_code;
    WebPPictureFree(&picture);

    if (!encodeOk)
    {
        WebPMemoryWriterClear(&writer);
        return luaL_error(L, "webp: WebPEncode failed (error code %d)", (int)errorCode);
    }

    lua_pushlstring(L, (const char*)writer.mem, writer.size);
    WebPMemoryWriterClear(&writer);

    return 1; // raw WebP bytes
}

// webp.get_info(image)
// image:   string with raw WebP bytes OR a file path
// returns: table { width, height, has_alpha, has_animation, lossy }
static int GetInfo(lua_State* L)
{
    const uint8_t* data  = nullptr;
    size_t         size  = 0;
    uint8_t*       owned = nullptr;

    if (!LoadData(L, 1, &data, &size, &owned))
    {
        return 0;
    }

    WebPBitstreamFeatures features;
    VP8StatusCode status = WebPGetFeatures(data, size, &features);

    if (owned) free(owned);

    if (GetWebPError(status))
    {
        return luaL_error(L, "webp: %s", GetWebPError(status));
    }

    lua_newtable(L);
    lua_pushinteger(L, features.width);         lua_setfield(L, -2, "width");
    lua_pushinteger(L, features.height);        lua_setfield(L, -2, "height");
    lua_pushboolean(L, features.has_alpha);     lua_setfield(L, -2, "has_alpha");
    lua_pushboolean(L, features.has_animation); lua_setfield(L, -2, "has_animation");
    lua_pushboolean(L, features.format == 1);   lua_setfield(L, -2, "lossy");

    return 1; // table info
}

static const luaL_reg Module_methods[] =
{
    {"decode",   Decode},
    {"encode",   Encode},
    {"get_info", GetInfo},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Module_methods);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result InitializeMyExtension(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(EXTENSION_NAME, LIB_NAME, 0, 0, InitializeMyExtension, 0, 0, 0)