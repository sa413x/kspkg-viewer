#include "render_extensions.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <cstdint>
#include <stb_image.h>

// Suppress nanosvg warning
#pragma warning( push )
#pragma warning( disable:4244 )

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg.h>
#include <nanosvgrast.h>

#pragma warning( pop )

#include <dxgi.h>
#include <d3d11.h>

// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-directx11-users

namespace ImGui {
    namespace RenderExtensions {
        namespace detail {
            ID3D11Device* device = nullptr;
        }

        void Setup( ID3D11Device* device ) {
            detail::device = device;
        }

        bool LoadTextureFromMemoryBitmap( const void* data, ID3D11ShaderResourceView** out_srv, int width, int height ) {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory( &desc, sizeof( desc ) );
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            ID3D11Texture2D* pTexture = nullptr;
            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = data;
            subResource.SysMemPitch = desc.Width * 4;
            subResource.SysMemSlicePitch = 0;
            if ( FAILED( detail::device->CreateTexture2D( &desc, &subResource, &pTexture ) ) ) {
                return false;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory( &srvDesc, sizeof( srvDesc ) );
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            if ( FAILED( detail::device->CreateShaderResourceView( pTexture, &srvDesc, out_srv ) ) ) {
                pTexture->Release();
                return false;
            }

            pTexture->Release();

            return true;
        }

        bool LoadTextureFromMemory( const void* data, const size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width,
                                    int* out_height ) {
            constexpr uint32_t svg_header = 0x6776733C;
            const uint32_t data_header = *static_cast< const uint32_t* >( data );

            // SVG
            if ( data_header == svg_header ) {
                const auto image = nsvgParse( static_cast< char* >( const_cast< void* >( data ) ), "px", 96 );
                const auto image_width = static_cast< int >( image->width );
                const auto image_height = static_cast< int >( image->height );

                const auto bitmap = static_cast< unsigned char* >( malloc( image_width * image_height * 4 ) );
                NSVGrasterizer* rast = nsvgCreateRasterizer();
                nsvgRasterize( rast, image, 0, 0, 1.0f, bitmap, image_width, image_height, image_width * 4 );

                nsvgDeleteRasterizer( rast );
                nsvgDelete( image );

                *out_width = image_width;
                *out_height = image_height;
                if ( !LoadTextureFromMemoryBitmap( bitmap, out_srv, image_width, image_height ) ) {
                    free( bitmap );
                    return false;
                }

                free( bitmap );
            }
            // PNG / JPG
            else {
                int image_width = 0;
                int image_height = 0;
                const auto image_data = stbi_load_from_memory( static_cast< const unsigned char* >( data ), static_cast< int >( data_size ),
                                                               &image_width, &image_height, nullptr, 4 );
                if ( image_data == nullptr )
                    return false;

                if ( !LoadTextureFromMemoryBitmap( image_data, out_srv, image_width, image_height ) ) {
                    stbi_image_free( image_data );
                    return false;
                }

                *out_width = image_width;
                *out_height = image_height;
                stbi_image_free( image_data );
            }

            return true;
        }
    } // namespace RenderExtensions
} // namespace ImGui