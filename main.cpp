#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include "d3dx12.h"
#include <mmreg.h>

using Microsoft::WRL::ComPtr;

#define DEBUG 1
#define FULLSCREEN 1

#include "shader/mzk.h"
#include "shader/gfx.h"

// -------------------------------------------------------------------------------------------------------------------------------------------
// cl.exe main.cpp /I .. /W4 /O1 /Os /GS- /wd4819 /wd4530 /wd4238  /link kernel32.lib user32.lib d3d12.lib dxgi.lib d3dcompiler.lib winmm.lib
// -------------------------------------------------------------------------------------------------------------------------------------------

#define SND_DURATION      80
#define SAMPLE_RATE    48000 
#define SND_NUMCHANNELS    2 
#define SND_NUMSAMPLES  ( SND_DURATION * SAMPLE_RATE )   
UINT sampleSize = sizeof(float)* SND_NUMSAMPLES * SND_NUMCHANNELS;

#define FRAMES           2

ComPtr<ID3D12Device> mDevice;
ComPtr<ID3D12CommandQueue> mCommandQueue;
ComPtr<IDXGISwapChain3> mSwapChain;

ComPtr<ID3D12Resource> mRenderTargets[FRAMES];

ComPtr<ID3D12DescriptorHeap>     mRtvHeap;
ComPtr<ID3D12DescriptorHeap> mResouceHeap;
UINT mRtvHeapSize;
UINT mResouceHeapSize;

ComPtr<ID3D12Fence> mFence;
HANDLE mFenceEvent;
UINT64 mFenceValue;

ComPtr<ID3D12RootSignature> mRootSignature;

ComPtr<ID3D12CommandAllocator>       mCmdAlloc;
ComPtr<ID3D12GraphicsCommandList>     mCmdList;
ComPtr<ID3DBlob>                  mVertexShader;
ComPtr<ID3DBlob>                   mPixelShader;
ComPtr<ID3DBlob>                    mMzkShader;
ComPtr<ID3D12PipelineState>               mPSO;
ComPtr<ID3D12PipelineState>            mMzkPSO;
ComPtr<ID3D12Resource>                mCBuffer;
ComPtr<ID3D12Resource>              mMzkBuffer;
ComPtr<ID3D12Resource>          readBackBuffer;
UINT8* mCBufferBegin;
// ++++++++++++++++++++++++

// #define  CompShaderCnt  8

ComPtr<ID3D12CommandAllocator>      mCompCmdAlloc[CompShaderCnt];
ComPtr<ID3D12GraphicsCommandList>    mCompCmdList[CompShaderCnt];
ComPtr<ID3DBlob>                      mCompShader[CompShaderCnt];
ComPtr<ID3D12PipelineState>              mCompPSO[CompShaderCnt];
ComPtr<ID3D12Resource>                mCompBuffer[CompShaderCnt];

enum class resourceSlot : int
{
    CBuf = 0,
    MzkBuf,
    UavBuf,
};

MSG msg;
float frameCount = 0.0f;


void WaitForPreviousFrame()
{
    const UINT64 fence = mFenceValue;
    mCommandQueue->Signal(mFence.Get(), fence);
    mFenceValue++;
    if (mFence->GetCompletedValue() < fence)
    {
        mFence->SetEventOnCompletion(fence, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}
    
int main()
{

	
#if FULLSCREEN
    int WIDTH =  1920;
    int HEIGHT = 1080;
    HWND hWnd = CreateWindow((LPCSTR)0xC018,0,WS_POPUP|WS_VISIBLE|WS_MAXIMIZE,0,0,0,0,0,0,0,0); /* Full Screan */
#else
    int WIDTH =  1920 / 2 ;
    int HEIGHT = 1080 / 2 ;
    HWND hWnd = CreateWindowExA(0,(LPCSTR)0xC018,0,WS_POPUP|WS_VISIBLE,50,50,WIDTH, HEIGHT,0,0,0,0);
#endif
    ShowCursor(0);

    static CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<FLOAT>(WIDTH), static_cast<FLOAT>(HEIGHT));
    static CD3DX12_RECT     scissorRect(0, 0, static_cast<LONG>(WIDTH), static_cast<LONG>(HEIGHT));
    
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf()));
    D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(mDevice.GetAddressOf()));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf()));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAMES;
    swapChainDesc.Width = WIDTH;
    swapChainDesc.Height = HEIGHT;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> swapChain;
    factory->CreateSwapChainForHwnd(mCommandQueue.Get(), hWnd, &swapChainDesc, NULL, NULL, swapChain.GetAddressOf());
    factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    swapChain.As(&mSwapChain);

    mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf()));
    mFenceValue = 1;
    mFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    { // Heap RTV
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = FRAMES;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf()));
    }
    { // Heap CBV_SRV_UAV
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = CompShaderCnt + 2;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mResouceHeap.GetAddressOf()));
    }
    
    mRtvHeapSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mResouceHeapSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
        handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT n = 0; n < FRAMES; n++)
        {
            mSwapChain->GetBuffer(n, IID_PPV_ARGS(mRenderTargets[n].GetAddressOf()));
            mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), NULL, handle);
            handle.ptr += mRtvHeapSize;
        }
    }

    // RootSignature
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,         1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC );
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, CompShaderCnt, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,         1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);


        CD3DX12_ROOT_PARAMETER1 rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1]);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2]);

         CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(
            _countof(rootParameters),
            rootParameters,
            0,
            NULL,
            D3D12_ROOT_SIGNATURE_FLAG_NONE
        );
        ComPtr<ID3DBlob>signature;
           D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, 0);
        mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf()));
    }

    // -------------------------------------------------------------------------
    
    // CommandList Init ..........
    mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCmdAlloc.GetAddressOf()));
    mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAlloc.Get(), NULL, IID_PPV_ARGS(mCmdList.GetAddressOf()));
    mCmdList->Close();
    
    for (UINT n = 0; n < CompShaderCnt; n++)
    {
        mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCompCmdAlloc[n].GetAddressOf()));
        mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCompCmdAlloc[n].Get(), NULL, IID_PPV_ARGS(mCompCmdList[n].GetAddressOf()));
        mCompCmdList[n]->Close();
    }

    // compile .................
	{
		HRESULT hr;
        ComPtr<ID3DBlob> errorMsg;
		hr =  D3DCompile(sGfx, strlen(sGfx), 0, 0, 0, "VSMain", "vs_5_0", 0, 0, mVertexShader.GetAddressOf(), errorMsg.GetAddressOf());
#if DEBUG
		if (FAILED(hr))
        {
            printf(static_cast<char*>(errorMsg->GetBufferPointer()));
            return 0;
        }
#endif
		
		hr = D3DCompile(sGfx, strlen(sGfx), 0, 0, 0, "PSMain", "ps_5_0", 0, 0, mPixelShader.GetAddressOf(), errorMsg.GetAddressOf());
		
		
#if DEBUG
        if (FAILED(hr))
        {
            printf(static_cast<char*>(errorMsg->GetBufferPointer()));
            return 0;
        }
#endif
		
    }
    
    for (UINT n = 0; n < CompShaderCnt; n++) 
    {
        const char* shaderSource =CompShaderSrc(n);
        ComPtr<ID3DBlob> errorMsg;

    	HRESULT hr = D3DCompile(shaderSource, strlen(shaderSource), 0, 0, 0, "CSMain", "cs_5_0", 0, 0, mCompShader[n].GetAddressOf(), errorMsg.GetAddressOf());
#if DEBUG
        if (FAILED(hr))
        {

            printf("SHADER ID : %i ..\n", n);
            printf(static_cast<char*>(errorMsg->GetBufferPointer()));
            return 0;
        }
#endif
    }

    {
        ComPtr<ID3DBlob> errorMsg;
    	HRESULT hr = D3DCompile(mzkShaderSource, strlen(mzkShaderSource), 0, 0, 0, "CSMain", "cs_5_0", 0, 0, mMzkShader.GetAddressOf(), errorMsg.GetAddressOf());
#if DEBUG
        if (FAILED(hr))
        {
            printf(static_cast<char*>(errorMsg->GetBufferPointer()));
            return 0;
        }
#endif
    }

    // Pipeline ........
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = mRootSignature.Get();
        desc.VS = CD3DX12_SHADER_BYTECODE(mVertexShader.Get());
        desc.PS = CD3DX12_SHADER_BYTECODE(mPixelShader.Get());
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // Culling
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        mDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(mPSO.GetAddressOf()));
    }

    for (UINT n = 0; n < CompShaderCnt; n++)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = mRootSignature.Get();       
        desc.CS = CD3DX12_SHADER_BYTECODE(mCompShader[n].Get());
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(mCompPSO[n].GetAddressOf()));
    }

    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = mRootSignature.Get();
        desc.CS = CD3DX12_SHADER_BYTECODE(mMzkShader.Get());
        desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(mMzkPSO.GetAddressOf()));
    }

    // Constant Buffer ...............
    { 
        mDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT/*256*/),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            IID_PPV_ARGS(mCBuffer.GetAddressOf()));

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
        handle = mResouceHeap->GetCPUDescriptorHandleForHeapStart();
        handle.Offset(static_cast<int>(resourceSlot::CBuf), mResouceHeapSize);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = mCBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT/*256*/;
        mDevice->CreateConstantBufferView(&cbvDesc, handle);
        mCBuffer->Map(0, NULL, reinterpret_cast<void**>(&mCBufferBegin));
    }

    // Compute Shader Buffer ..............................
    {  
        UINT bufSize = 0x400000; // 2048 * 2048
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.NumElements = bufSize;
        desc.Buffer.StructureByteStride = sizeof(float)*4;
        desc.Buffer.CounterOffsetInBytes = 0;
        desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        
        for (UINT n = 0; n < CompShaderCnt; n++)
        {
            mDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(
                    sizeof(float)*4*bufSize,
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                ),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                NULL,
                IID_PPV_ARGS(mCompBuffer[n].GetAddressOf())
            );
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
            handle = mResouceHeap->GetCPUDescriptorHandleForHeapStart();
            handle.Offset(static_cast<int>(resourceSlot::UavBuf) + n, mResouceHeapSize);
            mDevice->CreateUnorderedAccessView(mCompBuffer[n].Get(), NULL, &desc, handle);
        }
    }

    // Mzk Shader Buffer .........................
    { 
        mDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sampleSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            NULL,
            IID_PPV_ARGS(mMzkBuffer.GetAddressOf())
        );

           D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = SND_NUMSAMPLES;
            desc.Buffer.StructureByteStride = sizeof(float)*SND_NUMCHANNELS;
            desc.Buffer.CounterOffsetInBytes = 0;
            desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
        handle = mResouceHeap->GetCPUDescriptorHandleForHeapStart();
        handle.Offset(static_cast<int>(resourceSlot::MzkBuf), mResouceHeapSize);
        mDevice->CreateUnorderedAccessView(mMzkBuffer.Get(), NULL, &desc, handle);
    }
    
    // Mzk Setting .......................
    { 
        mDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sampleSize, D3D12_RESOURCE_FLAG_NONE) ,
            D3D12_RESOURCE_STATE_COPY_DEST,
            NULL,
            IID_PPV_ARGS(readBackBuffer.GetAddressOf())
        );
        mCmdAlloc->Reset();
        mCmdList->Reset(mCmdAlloc.Get(), mMzkPSO.Get());
        mCmdList->SetComputeRootSignature(mRootSignature.Get());
        ID3D12DescriptorHeap* ppHeaps[] = { mResouceHeap.Get() };
        mCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
           handle = mResouceHeap->GetGPUDescriptorHandleForHeapStart();
           handle.Offset(static_cast<INT>(resourceSlot::MzkBuf), mResouceHeapSize);
        mCmdList->SetComputeRootDescriptorTable(1, handle);
        mCmdList->Dispatch(SND_NUMSAMPLES / 32 + 1, 1, 1);

        mCmdList->ResourceBarrier(
            1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                mMzkBuffer.Get(), 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COPY_SOURCE 
            )
        );
        mCmdList->CopyResource(readBackBuffer.Get(), mMzkBuffer.Get());
        mCmdList->Close();

        ID3D12CommandList* ppCommandLists[] = { mCmdList.Get() };
        mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        WaitForPreviousFrame();    
    }

    
    void* pData;
    readBackBuffer->Map(0, NULL, &pData);
    float* samples = (float*)malloc(sampleSize);
    memcpy(samples, pData, sampleSize);
    readBackBuffer->Unmap(0, NULL);
    readBackBuffer->Release();

    WAVEFORMATEX wave_format = {
           WAVE_FORMAT_IEEE_FLOAT,
           SND_NUMCHANNELS,
           SAMPLE_RATE,
           SAMPLE_RATE*sizeof(float) * SND_NUMCHANNELS,
           sizeof(float) * SND_NUMCHANNELS,
           sizeof(float) * 8,
           0
    };
    WAVEHDR wave_hdr = {(LPSTR)samples, sampleSize};
    HWAVEOUT hWaveOut;
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wave_format, (DWORD_PTR)hWnd, 0, CALLBACK_WINDOW);
    waveOutPrepareHeader(hWaveOut, &wave_hdr, sizeof(wave_hdr));
    waveOutWrite(hWaveOut, &wave_hdr, sizeof(wave_hdr));
    MMTIME mmt = { TIME_SAMPLES };
    
    float time;

#if DEBUG
    float zero = 0.0f;
#endif

	// DescriptorHeap Settinng ......................
	ID3D12DescriptorHeap* ppHeaps[] = { mResouceHeap.Get() };

	CD3DX12_GPU_DESCRIPTOR_HANDLE cBufferHandle;
	cBufferHandle = mResouceHeap->GetGPUDescriptorHandleForHeapStart();
	cBufferHandle.Offset(static_cast<INT>(resourceSlot::CBuf), mResouceHeapSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE uavBufferHandle;
	uavBufferHandle = mResouceHeap->GetGPUDescriptorHandleForHeapStart();
	uavBufferHandle.Offset(static_cast<INT>(resourceSlot::UavBuf), mResouceHeapSize);
	
	for (UINT n = 0; n < 2; n++)
	{
		mCompCmdAlloc[n]->Reset();
		mCompCmdList[n]->Reset(mCompCmdAlloc[n].Get(), mCompPSO[n].Get());
		mCompCmdList[n]->SetComputeRootSignature(mRootSignature.Get());
		mCompCmdList[n]->SetDescriptorHeaps(1, ppHeaps);
		mCompCmdList[n]->SetComputeRootDescriptorTable(0, cBufferHandle);
		mCompCmdList[n]->SetComputeRootDescriptorTable(1, uavBufferHandle);
		mCompCmdList[n]->Dispatch(64 ,64 , 1);
		mCompCmdList[n]->Close();
	}

	
    
    do {
        PeekMessage(&msg, 0, 0, 0, TRUE);
        
        // Constant Buffer Setting ............
        waveOutGetPosition(hWaveOut, &mmt, sizeof(mmt));
        time = (float)mmt.u.sample / (float)SAMPLE_RATE;
        float cbuf[] = {(float)(WIDTH), (float)(HEIGHT), (float)mmt.u.sample / (float)SAMPLE_RATE, frameCount};
        memcpy(mCBufferBegin, cbuf, sizeof(float) * 4);
        frameCount++;
        
        
        // Sub Scene >> Compute Shader ........
        for (UINT n = 2; n < CompShaderCnt; n++)
        {
            mCompCmdAlloc[n]->Reset();
            mCompCmdList[n]->Reset(mCompCmdAlloc[n].Get(), mCompPSO[n].Get());
            mCompCmdList[n]->SetComputeRootSignature(mRootSignature.Get());
            mCompCmdList[n]->SetDescriptorHeaps(1, ppHeaps);
            mCompCmdList[n]->SetComputeRootDescriptorTable(0, cBufferHandle);
            mCompCmdList[n]->SetComputeRootDescriptorTable(1, uavBufferHandle);
            mCompCmdList[n]->Dispatch(60 ,34 , 1);
            mCompCmdList[n]->Close();
        }

        // Display ...................
        { 
            UINT frameIndex = mSwapChain->GetCurrentBackBufferIndex();
            mCmdAlloc->Reset();
            mCmdList->Reset(mCmdAlloc.Get(), mPSO.Get());
            mCmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[frameIndex].Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET));
            mCmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(mCompBuffer[0].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ));
            mCmdList->RSSetViewports(1, &viewport);
            mCmdList->RSSetScissorRects(1, &scissorRect);            
            mCmdList->SetGraphicsRootSignature(mRootSignature.Get());
            mCmdList->SetDescriptorHeaps(1, ppHeaps);
            mCmdList->SetGraphicsRootDescriptorTable(0, cBufferHandle);
            mCmdList->SetGraphicsRootDescriptorTable(2, uavBufferHandle);
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, mRtvHeapSize);
            mCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
            mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mCmdList->DrawInstanced(6, 1, 0, 0);
            mCmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(mCompBuffer[0].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
            mCmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[frameIndex].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT));
            mCmdList->Close();
        }

        // Command Queue ................
        
        ID3D12CommandList* ppCommandLists[CompShaderCnt + 1];
        for (UINT n = 0; n < CompShaderCnt; n++)
        {
            ppCommandLists[n] = mCompCmdList[n].Get();
        }
        ppCommandLists[CompShaderCnt] =  mCmdList.Get();
        
        mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        mSwapChain->Present(1, 0);
        WaitForPreviousFrame();
        
#if DEBUG
        if( int(frameCount) % 10 == 0)
            printf("\rFPS : %.1f TIME : %.1f", 1.0f / (time - zero), time);
        zero = time;
#endif
        
    } while (!GetAsyncKeyState(VK_ESCAPE) && (mmt.u.sample < SND_NUMSAMPLES));
    
    // finsh ..........
    waveOutReset(hWaveOut);
    WaitForPreviousFrame();
    CloseHandle(mFenceEvent);
    return 0;
}
