#pragma once

struct ID3D11Device;
struct ID3D11ShaderResourceView;

namespace ImGui {
    namespace RenderExtensions {
        void Setup( ID3D11Device* device_ctx );
        bool LoadTextureFromMemory( const void* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width,
                                    int* out_height );
    } // namespace RenderExtensions
} // namespace ImGui