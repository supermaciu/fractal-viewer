#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

// Vertex structure for full-screen quad
struct Vertex {
    float x, y;     // position
};

// Full-screen quad vertices (4 vertices shared by 2 triangles)
static struct Vertex vertices[] = {
    {-1.0,  1.0},  // 0: top-left
    {-1.0, -1.0},  // 1: bottom-left
    { 1.0, -1.0},  // 2: bottom-right
    { 1.0,  1.0},  // 3: top-right
};

// Index buffer: defines two triangles using the 4 vertices
static Uint16 indices[] = {
    0, 1, 2,  // First triangle
    0, 2, 3   // Second triangle
};

static double zoom = 1;

SDL_Window* window;
SDL_GPUDevice* device;
SDL_GPUBuffer* vertexBuffer;
SDL_GPUBuffer* indexBuffer;
SDL_GPUBuffer* storageBuffer;
SDL_GPUGraphicsPipeline* graphicsPipeline;
bool needsRender = true;

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

void syncStorageBuffer(SDL_Event *event) {
    // Upload resolution data
    SDL_GPUTransferBufferCreateInfo storageTransferInfo = {0};
    storageTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    storageTransferInfo.size = 5 * sizeof(double);
    
    SDL_GPUTransferBuffer* storageTransferBuffer = SDL_CreateGPUTransferBuffer(device, &storageTransferInfo);
    
    int width, height, mouseX = 0, mouseY = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    
    if (event) {
        mouseX = event->wheel.mouse_x;
        mouseY = event->wheel.mouse_y;
    }

    double buffer[5] = {(double)width, (double)height, zoom, (double)mouseX, (double)mouseY};

    void* storageMap = SDL_MapGPUTransferBuffer(device, storageTransferBuffer, false);
    SDL_memcpy(storageMap, buffer, 5 * sizeof(double));
    SDL_UnmapGPUTransferBuffer(device, storageTransferBuffer);
    
    // Upload storage buffer data to GPU
    SDL_GPUCommandBuffer* storageUploadCmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* storageCopyPass = SDL_BeginGPUCopyPass(storageUploadCmd);
    
    SDL_GPUTransferBufferLocation storageSrc = {0};
    storageSrc.transfer_buffer = storageTransferBuffer;
    storageSrc.offset = 0;
    
    SDL_GPUBufferRegion storageDst = {0};
    storageDst.buffer = storageBuffer;
    storageDst.offset = 0;
    storageDst.size = 5 * sizeof(double);
    
    SDL_UploadToGPUBuffer(storageCopyPass, &storageSrc, &storageDst, false);
    SDL_EndGPUCopyPass(storageCopyPass);
    SDL_SubmitGPUCommandBuffer(storageUploadCmd);
    
    SDL_ReleaseGPUTransferBuffer(device, storageTransferBuffer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // Create a window
    window = SDL_CreateWindow("Fractal Viewer", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Create the GPU device - prefer SPIRV for cross-platform, fallback to others
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!device) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Determine which shader format to use
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    const char* vertexShaderPath = "shaders/vertex.spv";
    const char* fragmentShaderPath = "shaders/fragment.spv";
    
    // Load the vertex shader code
    size_t vertexCodeSize; 
    void* vertexCode = SDL_LoadFile(vertexShaderPath, &vertexCodeSize);
    if (!vertexCode) {
        SDL_Log("Failed to load vertex shader: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the vertex shader
    SDL_GPUShaderCreateInfo vertexInfo = {0};
    vertexInfo.code = (Uint8*)vertexCode;
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint = "main";
    vertexInfo.format = format;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(device, &vertexInfo);
    SDL_free(vertexCode);
    
    if (!vertexShader) {
        SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Load the fragment shader code
    size_t fragmentCodeSize; 
    void* fragmentCode = SDL_LoadFile(fragmentShaderPath, &fragmentCodeSize);
    if (!fragmentCode) {
        SDL_Log("Failed to load fragment shader: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the fragment shader
    SDL_GPUShaderCreateInfo fragmentInfo = {0};
    fragmentInfo.code = (Uint8*)fragmentCode;
    fragmentInfo.code_size = fragmentCodeSize;
    fragmentInfo.entrypoint = "main";
    fragmentInfo.format = format;
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 1;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);
    SDL_free(fragmentCode);
    
    if (!fragmentShader) {
        SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create the graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    // Describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0].slot = 0;
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch = sizeof(struct Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescriptions;

    // Describe the vertex attributes
    SDL_GPUVertexAttribute vertexAttributes[1];

    // position attribute
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = 0;

    pipelineInfo.vertex_input_state.num_vertex_attributes = 1;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // Set the render target format
    SDL_GPUColorTargetDescription colorTarget = {0};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;

    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!graphicsPipeline) {
        SDL_Log("Failed to create graphics pipeline: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Release shaders (they're now part of the pipeline)
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // Create vertex buffer
    SDL_GPUBufferCreateInfo bufferInfo = {0};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = sizeof(vertices);
    
    vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!vertexBuffer) {
        SDL_Log("Failed to create vertex buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create transfer buffer to upload vertex data
    SDL_GPUTransferBufferCreateInfo transferInfo = {0};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(vertices);
    
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    
    // Map and copy vertex data
    void* map = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(map, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);
    
    // Upload to GPU
    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);
    
    SDL_GPUTransferBufferLocation src = {0};
    src.transfer_buffer = transferBuffer;
    src.offset = 0;
    
    SDL_GPUBufferRegion dst = {0};
    dst.buffer = vertexBuffer;
    dst.offset = 0;
    dst.size = sizeof(vertices);
    
    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    
    // Create index buffer
    SDL_GPUBufferCreateInfo indexBufferInfo = {0};
    indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferInfo.size = sizeof(indices);
    
    indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferInfo);
    if (!indexBuffer) {
        SDL_Log("Failed to create index buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Create transfer buffer for indices
    SDL_GPUTransferBufferCreateInfo indexTransferInfo = {0};
    indexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    indexTransferInfo.size = sizeof(indices);
    
    SDL_GPUTransferBuffer* indexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &indexTransferInfo);
    
    // Map and copy index data
    void* indexMap = SDL_MapGPUTransferBuffer(device, indexTransferBuffer, false);
    SDL_memcpy(indexMap, indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, indexTransferBuffer);
    
    // Upload indices to GPU
    SDL_GPUTransferBufferLocation indexSrc = {0};
    indexSrc.transfer_buffer = indexTransferBuffer;
    indexSrc.offset = 0;
    
    SDL_GPUBufferRegion indexDst = {0};
    indexDst.buffer = indexBuffer;
    indexDst.offset = 0;
    indexDst.size = sizeof(indices);
    
    SDL_UploadToGPUBuffer(copyPass, &indexSrc, &indexDst, false);
    
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmd);
    
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(device, indexTransferBuffer);

    // Create storage buffer for window resolution
    SDL_GPUBufferCreateInfo storageBufferInfo = {0};
    storageBufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    storageBufferInfo.size = 5 * sizeof(double);  // vec2: width, height; vec3: zoom, mouse_x, mouse_y
    
    storageBuffer = SDL_CreateGPUBuffer(device, &storageBufferInfo);
    if (!storageBuffer) {
        SDL_Log("Failed to create storage buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // // Upload resolution data
    syncStorageBuffer(NULL);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Only render if event occured
    if (!needsRender) {
        return SDL_APP_CONTINUE;
    }
    
    SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!cmdBuffer) {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(cmdBuffer);
        return SDL_APP_CONTINUE;
    }
    
    if (swapchainTexture) {
        // Setup render target
        SDL_GPUColorTargetInfo colorTarget = {0};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color.r = 0.0f;
        colorTarget.clear_color.g = 0.0f;
        colorTarget.clear_color.b = 0.0f;
        colorTarget.clear_color.a = 1.0f;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        
        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, NULL);
        
        SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);
        
        // Bind vertex buffer
        SDL_GPUBufferBinding vertexBinding = {0};
        vertexBinding.buffer = vertexBuffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
        
        // Bind index buffer
        SDL_GPUBufferBinding indexBinding = {0};
        indexBinding.buffer = indexBuffer;
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        
        // Bind storage buffer
        SDL_GPUBuffer* storageBuffers[] = {storageBuffer};
        SDL_BindGPUFragmentStorageBuffers(renderPass, 0, storageBuffers, 1);
        
        // Draw full-screen quad (4 vertices, 6 indices = 2 triangles)
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
        
        SDL_EndGPURenderPass(renderPass);
    }
    
    SDL_SubmitGPUCommandBuffer(cmdBuffer);
    
    needsRender = false;
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        return SDL_APP_SUCCESS;
    }
    
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == 1) {

    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        zoom += -event->wheel.y * zoom * pow(2, -event->wheel.y/2-0.5); // inaccuracies when fast scrolling, lagging
        syncStorageBuffer(event);
        needsRender = true;
    }

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        syncStorageBuffer(NULL);
        needsRender = true;
    }
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (vertexBuffer) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
    }
    if (indexBuffer) {
        SDL_ReleaseGPUBuffer(device, indexBuffer);
    }
    if (storageBuffer) {
        SDL_ReleaseGPUBuffer(device, storageBuffer);
    }
    if (graphicsPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);
    }
    if (device) {
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
}
