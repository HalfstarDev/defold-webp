# defold-webp

A Defold extension for using WebP images. Uses [libwebp-1.6.0](https://github.com/webmproject/libwebp).

## Installation

This asset can be added as a [library dependency](https://defold.com/manuals/libraries/#setting-up-library-dependencies) in your project:

* Use a specific version for development and release to avoid breaking changes: https://github.com/HalfstarDev/defold-webp/releases
* Use the latest version only while evaluating and testing the asset: https://github.com/HalfstarDev/defold-webp/archive/master.zip

For Android, the minimum SDK version is 21.

## Usage

#### webp.decode(image [, options])

Decode a WebP image into an image buffer. Use optional Lua table with [WebPDecoderOptions](https://developers.google.com/speed/webp/docs/api#advanced_decoding_api) as keys. Returns buffer, width, and height.

```
local buf, w, h = webp.decode(sys.load_resource("/assets/image_1.webp"))
resource.set_texture(go.get("#sprite", "texture0"), {
    width        = w,
    height       = h,
    type         = resource.TEXTURE_TYPE_2D,
    format       = resource.TEXTURE_FORMAT_RGBA,
    num_mip_maps = 1
}, buf)
```

#### webp.encode(buffer, width, height [, options])

Encode a image buffer into a WebP image. Use optional Lua table with [WebPConfig](https://developers.google.com/speed/webp/docs/api#advanced_encoding_api) fields as keys. Returns string with WebP image.

```
local buf, w, h = webp.decode(image)
local webp_image = webp.encode(buf, w, h, {quality = 50})
local f = io.open("out.webp", "wb")
if f then
    f:write(webp_image)
    f:close()
end
```

#### webp.get_info(image)

Get information about a WebP image. Returns table with following fields: width, height, has_alpha, has_animation, lossy.
```
local info = webp.get_info(image)
print(info.width, info.height)
```
