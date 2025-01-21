#include "render_extensions.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

        bool LoadTextureFromMemory( const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width,
                                    int* out_height ) {
            int image_width = 0;
            int image_height = 0;
            unsigned char* image_data =
                stbi_load_from_memory( ( const unsigned char* )data, ( int )data_size, &image_width, &image_height, NULL, 4 );
            if ( image_data == NULL )
                return false;

            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory( &desc, sizeof( desc ) );
            desc.Width = image_width;
            desc.Height = image_height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            ID3D11Texture2D* pTexture = NULL;
            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = image_data;
            subResource.SysMemPitch = desc.Width * 4;
            subResource.SysMemSlicePitch = 0;
            detail::device->CreateTexture2D( &desc, &subResource, &pTexture );

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory( &srvDesc, sizeof( srvDesc ) );
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            detail::device->CreateShaderResourceView( pTexture, &srvDesc, out_srv );
            pTexture->Release();

            *out_width = image_width;
            *out_height = image_height;
            stbi_image_free( image_data );

            return true;
        }
    } // namespace RenderExtensions
} // namespace ImGui