/*

  Enumerating Adapters (Graphics Cards) and Outputs (Monitors)
  ------------------------------------------------------------
   - Use IDXGIFactory, EnumAdapters.
  - For each enumerated IDXGIAdapter, use EnumOutputs to get the IDXGIOutput (Monitors)

  Duplicate Output
  ----------------
  - From a IDGIOutput which you can retrieve from an IDXGIAdapter you 
    need to query the IDXGIOutput1 interface which exposes the 
    IDXGIOutputDuplication api.
  - You call IDXGIOutput1::DuplicateOutput() and pass a Direct3D 
    device for the first parameter. 
 
  Receiving desktop images
  ------------------------
  - Once you have a IDXGIOutputDuplication handle you can start 
    requesting (acquiring) frames. You use the AcquireNextFrame(), 
    on the duplication handle. When you receive a S_OK it means you
    received a new frame. 
  - The received frame is stored in a IDXGIResource, which you need
    to release (isn't documented, but found this inthe examples).
  - You can get access to the texture that the IDXGIResource wraps
    around by querying the ID3D11Texture2D interface on this object.
 
  
  References:
  -----------
  - Code Project, 3 solutions: http://www.codeproject.com/Articles/5051/Various-methods-for-capturing-the-screen
  - SO, see comment of Herman: http://stackoverflow.com/questions/5069104/fastest-method-of-screen-capturing
  - A blog post on DirectX, from above SO: http://blog.nektra.com/main/2013/07/23/instrumenting-direct3d-applications-to-capture-video-and-calculate-frames-per-second/
  - Mirror Driver: https://msdn.microsoft.com/en-us/library/windows/hardware/ff568315(v=vs.85).aspx
    ^
    |
    +--- This supposed to be the best solution according to SO and the author of the Code Project article.
  - Desktop Duplication API (win8): http://blogs.msdn.com/b/dsui_team/archive/2013/03/25/ways-to-capture-the-screen.aspx
    + Doesn't work for all full screen apps. 
  - Some blog with a couple of methods: http://blogs.msdn.com/b/dsui_team/archive/2013/03/25/ways-to-capture-the-screen.aspx
  - A nice clean, simple example of reading frames using the duplication api: https://github.com/lwnexgen/Dx11Streamer/blob/master/Dx11Streamer/Main.cpp
    Backup: https://gist.github.com/roxlu/b5689d9ad2eda095b793
  - How to map the pixels you receive from the duplication api: : http://www.getcodesamples.com/src/2D4BC0AA/BFBE14E5

 */
#include <DXGI.h>
#include <DXGI1_2.h> /* For IDXGIOutput1 */
#include <D3D11.h> /* For the D3D11* interfaces. */

struct CaptureContext {
    IDXGIFactory1* factory;
    IDXGIAdapter1* adapter;
    IDXGIAdapter1* adapters[32]; /* Needs to be Released(). */
    u32 adapters_count;
    
    IDXGIOutput* output;
    IDXGIOutput* outputs[32]; /* Needs to be Released(). */
    u32 outputs_count;
    
    ID3D11Device* d3d_device; /* Needs to be released. */
    ID3D11DeviceContext* d3d_context; /* Needs to be released. */
    IDXGIAdapter1* d3d_adapter;
    D3D_FEATURE_LEVEL d3d_feature_level; /* The selected feature level (D3D version), selected from the Feature Levels array, which is NULL here; when it's NULL the default list is used see:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff476082%28v=vs.85%29.aspx ) */
    
    IDXGIOutput1* output1;
    IDXGIOutputDuplication* duplication;
    ID3D11Texture2D* staging_tex;
    
    D3D11_TEXTURE2D_DESC tex_desc;
};

bool dx_destroy(CaptureContext *context)
{
    /* Cleanup */
    {
        if (NULL != context->staging_tex) {
            context->staging_tex->Release();
            context->staging_tex = NULL;
        }
        
        if (NULL != context->d3d_device) {
            context->d3d_device->Release();
            context->d3d_device = NULL;
        }
        
        if (NULL != context->d3d_context) {
            context->d3d_context->Release();
            context->d3d_context = NULL;
        }
        
        if (NULL != context->duplication) {
            context->duplication->Release();
            context->duplication = NULL;
        }
        
        for (size_t i = 0; i < context->adapters_count; ++i) {
            if (NULL != context->adapters[i]) {
                context->adapters[i]->Release();
                context->adapters[i] = NULL;
            }
        }
        
        for (size_t i = 0; i < context->outputs_count; ++i) {
            if (NULL != context->outputs[i]) {
                context->outputs[i]->Release();
                context->outputs[i] = NULL;
            }
        }
        
        if (NULL != context->output1) {
            context->output1->Release();
            context->output1 = NULL;
        }
        
        if (NULL != context->factory) {
            context->factory->Release();
            context->factory = NULL;
        }
    }
    
    return true;
}

bool dx_init(CaptureContext *context)
{
    memset(context, 0, sizeof(CaptureContext));
    
    /* Retrieve a IDXGIFactory that can enumerate the adapters. */
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&context->factory));
    
    if (S_OK != hr) {
        printf("Error: failed to retrieve the IDXGIFactory.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Enumerate the adapters.*/
    UINT ac = 0;
    while (DXGI_ERROR_NOT_FOUND != context->factory->EnumAdapters1(ac, &context->adapter)) {
        context->adapters[context->adapters_count++] = context->adapter;
        ++ac;
    }
    
    for (u32 i = 0; i < context->adapters_count; ++i) {
        
        context->adapter = context->adapters[i];
        
        u32 output_index = 0;
        while (DXGI_ERROR_NOT_FOUND != context->adapter->EnumOutputs(output_index, &context->output)) {
            printf("Found monitor %d on adapter: %u\n", output_index, i);
            
            DXGI_OUTPUT_DESC desc;
            context->output->GetDesc(&desc);
            if (desc.AttachedToDesktop)
            {
                context->outputs[context->outputs_count++] = context->output;
            }
            
            ++output_index;
        }
    }
    
    if (0 >= context->outputs_count) {
        printf("Error: no outputs found (%u).\n", context->outputs_count);
        exit(EXIT_FAILURE);
    }
    
    /*
  
      To get access to a OutputDuplication interface we need to have a 
      Direct3D device which handles the actuall rendering and "gpu" 
      stuff. According to a gamedev stackexchange it seems we can create
      one w/o a HWND. 
  
     */
    
    
    { /* Start creating a D3D11 device */
        
#if 1
        /* 
           NOTE:  Apparently the D3D11CreateDevice function returns E_INVALIDARG, when
                  you pass a pointer to an adapter for the first parameter and use the 
                  D3D_DRIVER_TYPE_HARDWARE. When you want to pass a valid pointer for the
                  adapter, you need to set the DriverType parameter (2nd) to 
                  D3D_DRIVER_TYPE_UNKNOWN.
                 
                  @todo figure out what would be the best solution; easiest to use is 
                  probably using NULL here. 
         */
        
        u32 use_adapter = 0;
        context->d3d_adapter = context->adapters[use_adapter];
        if (NULL == context->d3d_adapter) {
            printf("Error: the stored adapter is NULL.\n");
            exit(EXIT_FAILURE);
        }
#endif
        
        hr = D3D11CreateDevice(context->d3d_adapter,              /* Adapter: The adapter (video card) we want to use. We may use NULL to pick the default adapter. */  
                               D3D_DRIVER_TYPE_UNKNOWN,  /* DriverType: We use the GPU as backing device. */
                               NULL,                     /* Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicaple. */
                               NULL,                     /* Flags: maybe we need to use D3D11_CREATE_DEVICE_BGRA_SUPPORT because desktop duplication is using this. */
                               NULL,                     /* Feature Levels (ptr to array):  what version to use. */
                               0,                        /* Number of feature levels. */
                               D3D11_SDK_VERSION,        /* The SDK version, use D3D11_SDK_VERSION */
                               &context->d3d_device,              /* OUT: the ID3D11Device object. */
                               &context->d3d_feature_level,       /* OUT: the selected feature level. */
                               &context->d3d_context);            /* OUT: the ID3D11DeviceContext that represents the above features. */
        
        if (S_OK != hr) {
            printf("Error: failed to create the D3D11 Device.\n");
            if (E_INVALIDARG == hr) {
                printf("Got INVALID arg passed into D3D11CreateDevice. Did you pass a adapter + a driver which is not the UNKNOWN driver?.\n");
            }
            exit(EXIT_FAILURE);
        }
        
    } /* End creating a D3D11 device. */
    
    /* 
       Create a IDXGIOutputDuplication for the first monitor. 
       
       - From a IDXGIOutput which represents an monitor, we query a IDXGIOutput1
         because the IDXGIOutput1 has the DuplicateOutput feature. 
    */
    
    
    { /* Start IDGIOutputDuplication init. */
        
        u32 use_monitor = 0;
        context->output = context->outputs[use_monitor];
        if (use_monitor >= context->outputs_count || NULL == context->output) {
            printf("No valid output found. The output is NULL.\n");
            exit(EXIT_FAILURE);
        }
        
        hr = context->output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&context->output1);
        if (S_OK != hr) {
            printf("Error: failed to query the IDXGIOutput1 interface.\n");
            exit(EXIT_FAILURE);
        }
        
        hr = context->output1->DuplicateOutput(context->d3d_device, &context->duplication);
        if (S_OK != hr) {
            printf("Error: failed to create the duplication output.\n");
            exit(EXIT_FAILURE);
        }
        printf("Queried the IDXGIOutput1.\n");
        
    } /* End IDGIOutputDuplication init. */
    
    if (NULL == context->duplication) {
        printf("Error: okay, we shouldn't arrive here but the duplication var is NULL.\n");
        exit(EXIT_FAILURE);
    }
    
    /*
      To download the pixel data from the GPU we need a 
      staging texture. Therefore we need to determine the width and 
      height of the buffers that we receive. 
      
      @TODO - We could also retrieve the width/height from the texture we got 
              from through the acquired frame (see the 'tex' variable below).
              That may be a safer solution.
    */
    DXGI_OUTPUT_DESC output_desc;
    {
        hr = context->output->GetDesc(&output_desc);
        if (S_OK != hr) {
            printf("Error: failed to get the DXGI_OUTPUT_DESC from the output (monitor). We need this to create a staging texture when downloading the pixels from the gpu.\n");
            exit(EXIT_FAILURE);
        }
        
        printf("The monitor has the following dimensions: left: %d, right: %d, top: %d, bottom: %d.\n"
               ,(int)output_desc.DesktopCoordinates.left
               ,(int)output_desc.DesktopCoordinates.right
               ,(int)output_desc.DesktopCoordinates.top
               ,(int)output_desc.DesktopCoordinates.bottom
               );
    }
    
    if (0 == output_desc.DesktopCoordinates.right
        || 0 == output_desc.DesktopCoordinates.bottom)
    {
        printf("The output desktop coordinates are invalid.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Create the staging texture that we need to download the pixels from gpu. */
    context->tex_desc.Width = output_desc.DesktopCoordinates.right;
    context->tex_desc.Height = output_desc.DesktopCoordinates.bottom;
    context->tex_desc.MipLevels = 1;
    context->tex_desc.ArraySize = 1; /* When using a texture array. */
    context->tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
    context->tex_desc.SampleDesc.Count = 1; /* MultiSampling, we can use 1 as we're just downloading an existing one. */
    context->tex_desc.SampleDesc.Quality = 0; /* "" */
    context->tex_desc.Usage = D3D11_USAGE_STAGING;
    context->tex_desc.BindFlags = 0;
    context->tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    context->tex_desc.MiscFlags = 0;
    
    
    hr = context->d3d_device->CreateTexture2D(&context->tex_desc, NULL, &context->staging_tex);
    if (E_INVALIDARG == hr) {
        printf("Error: received E_INVALIDARG when trying to create the texture.\n");
        exit(EXIT_FAILURE);
    }
    else if (S_OK != hr) {
        printf("Error: failed to create the 2D texture, error: %d.\n", hr);
        exit(EXIT_FAILURE);
    }
    
    /* 
       Get some info about the output duplication. 
       When the DesktopImageInSystemMemory is TRUE you can use 
       the MapDesktopSurface/UnMapDesktopSurface directly to retrieve the
       pixel data. If not, then you need to use a surface. 
  
    */
    DXGI_OUTDUPL_DESC duplication_desc;
    context->duplication->GetDesc(&duplication_desc);
    printf("duplication desc.DesktopImageInSystemMemory: %c\n", (duplication_desc.DesktopImageInSystemMemory) ? 'y' : 'n');
    
    return true;
}

int dx_capture(CaptureContext *context, u8 *image_data, u32 size, u32 *width, u32 *height) {
    /* Access a couple of frames. */
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    IDXGIResource* desktop_resource = NULL;
    ID3D11Texture2D* tex = NULL;
    DXGI_MAPPED_RECT mapped_rect;
    
    for (int i = 0; i < 1; ++i) {
        //  printf("%02d - ", i);
        
        HRESULT hr = context->duplication->AcquireNextFrame(500, &frame_info, &desktop_resource);
        if (DXGI_ERROR_ACCESS_LOST == hr) {
            printf("Received a DXGI_ERROR_ACCESS_LOST.\n");
        }
        else if (DXGI_ERROR_WAIT_TIMEOUT == hr) {
            printf("Received a DXGI_ERROR_WAIT_TIMEOUT.\n");
        }
        else if (DXGI_ERROR_INVALID_CALL == hr) {
            printf("Received a DXGI_ERROR_INVALID_CALL.\n");
        }
        else if (S_OK == hr) {
            //printf("Yay we got a frame.\n");
            
            /* Print some info. */
            //printf("frame_info.TotalMetadataBufferSize: %u\n", frame_info.TotalMetadataBufferSize);
            //printf("frame_info.AccumulatedFrames: %u\n", frame_info.AccumulatedFrames);
            
            /* Get the texture interface .. */
#if 1      
            hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
            if (S_OK != hr) {
                printf("Error: failed to query the ID3D11Texture2D interface on the IDXGIResource we got.\n");
                exit(EXIT_FAILURE);
            }
#endif      
            
            /* Map the desktop surface */
            hr = context->duplication->MapDesktopSurface(&mapped_rect);
            if (S_OK == hr) {
                printf("We got acess to the desktop surface\n");
                hr = context->duplication->UnMapDesktopSurface();
                if (S_OK != hr) {
                    printf("Error: failed to unmap the desktop surface after successfully mapping it.\n");
                }
            }
            else if (DXGI_ERROR_UNSUPPORTED == hr) {
                //printf("MapDesktopSurface returned DXGI_ERROR_UNSUPPORTED.\n");
                /* 
                   According to the docs, when we receive this error we need
                   to transfer the image to a staging surface and then lock the 
                    image by calling IDXGISurface::Map().
        
                   To get the data from GPU to the CPU, we do:
        
                       - copy the frame into our staging texture
                       - map the texture 
                       - ... do something 
                       - unmap.
        
                   @TODO figure out what solution is faster:
        
                   There are multiple solutions to copy a texture. I have 
                   to look into what solution is better. 
                   -  d3d_context->CopySubresourceRegion();
                   -  d3d_context->CopyResource(dest, src)
        
                   @TODO we need to make sure that the width/height are valid. 
         
                */
                
                context->d3d_context->CopyResource(context->staging_tex, tex);
                
                D3D11_MAPPED_SUBRESOURCE map;
                HRESULT map_result = context->d3d_context->Map(context->staging_tex,          /* Resource */
                                                               0,                    /* Subresource */ 
                                                               D3D11_MAP_READ,       /* Map type. */
                                                               0,                    /* Map flags. */
                                                               &map);
                
                if (S_OK == map_result) {
                    unsigned char* data = (unsigned char*)map.pData;
                    //printf("Mapped the staging tex; we can access the data now.\n");
                    printf("RowPitch: %u, DepthPitch: %u, %02X, %02X, %02X\n", map.RowPitch, map.DepthPitch, data[0], data[1], data[2]);
                    
                    *width = context->tex_desc.Width;
                    *height = context->tex_desc.Height;
                    
                    u32 copy_size = context->tex_desc.Width * context->tex_desc.Height * 4;
                    if (size >= copy_size /*&& data[0] == 0xFF*/)
                    {
                        memcpy(image_data, data, copy_size);
                    }
#if 0
                    if (i < 25) {
                        char fname[512];
                        
                        /* We have to make the image opaque. */
                        
                        for (int k = 0; k < tex_desc.Width; ++k) {
                            for (int l = 0; l < tex_desc.Height; ++l) {
                                int dx = l * tex_desc.Width * 4 + k * 4;
                                data[dx + 3] = 0xFF;
                            }
                        }
                        sprintf(fname, "capture_%03d.png", i);
                        save_png(fname,
                                 tex_desc.Width, tex_desc.Height, 8, PNG_COLOR_TYPE_RGBA,
                                 (unsigned char*)map.pData, map.RowPitch, PNG_TRANSFORM_BGR);
                    }
#endif
                }
                else {
                    printf("Error: failed to map the staging tex. Cannot access the pixels.\n");
                }
                
                context->d3d_context->Unmap(context->staging_tex, 0);
            }
            else if (DXGI_ERROR_INVALID_CALL == hr) {
                printf("MapDesktopSurface returned DXGI_ERROR_INVALID_CALL.\n");
            }
            else if (DXGI_ERROR_ACCESS_LOST == hr) {
                printf("MapDesktopSurface returned DXGI_ERROR_ACCESS_LOST.\n");
            }
            else if (E_INVALIDARG == hr) {
                printf("MapDesktopSurface returned E_INVALIDARG.\n");
            }
            else {
                printf("MapDesktopSurface returned an unknown error.\n");
            }
        }
        
        /* Clean up */
        {
            
            if (NULL != tex) {
                tex->Release();
                tex = NULL;
            }
            
            if (NULL != desktop_resource) {
                desktop_resource->Release();
                desktop_resource = NULL;
            }
            
            /* We must release the frame. */
            hr = context->duplication->ReleaseFrame();
            if (S_OK != hr) {
                printf("Failed to release the duplication frame.\n");
            }
        }
    }
    
    //printf("Monitors connected to adapter: %lu\n", i);
    
    
    return 0;
}


