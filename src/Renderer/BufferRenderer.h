#ifndef BUFFER_RENDERER
#define BUFFER_RENDERER

#include<Windows.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#define SDL_MAIN_HANDLED
#define NDEBUG 


#define TEST_FOLDER_NAME (std::string)"Completed Tests"

#include<SDL/SDL.h>
#include<cstdint>
#include<string>
#include <thread>
#include <future>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <filesystem>
#include <cstdio>
#include<io.h>
#include<stdio.h>
#include<iostream>
#include<direct.h>
#include <cstring>

#include<tinyXml2/tinyxml2.h>

#include <assert.h>
#include <stdio.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")


//#define _DEBUG

struct WindowsProperties
{
	UINT16 width = 0;
	UINT16 height = 0;
	std::string windowName = "Default Name";

};

struct CommandPointers
{
	ID3D12CommandQueue* Queue;
	ID3D12GraphicsCommandList4* List;
	ID3D12CommandAllocator* Allocator;

	std::string QueueName{""};
	UINT64 CopySize{0};
};

/*
	50:(1mb,1mb):(1mb,0mb)
	100:(2mb,2mb):(2mb,0mb)
	150:(3mb,3mb):(3mb,0mb)
	200:(4mb,4mb):(4mb,0mb)
*/

struct ResourceDesc
{
	//Resource Info
	std::string Name{ "" };
	uint64_t DataSize{ 0 };

	//Allocation Info
	D3D12_HEAP_TYPE HeapType{};
	D3D12_RESOURCE_STATES HeapFinalState{};
};


struct TimeStamp
{
	std::string Queue{""};
	std::string HeapFromTo{""};
	double Time;
	UINT64 DataSize;

};

const unsigned int NR_OF_QUEUES = 3;


class BufferRenderer
{

private:

	HWND m_HWND = {};
	SDL_Event m_Events;
	SDL_Window* m_SDLWindow = nullptr;
	WindowsProperties m_WindowProperties = {};

	ID3D12Device5* m_Device;
	IDXGIAdapter3* m_Adapter = nullptr;

	//Direct, Copy, Compute
	ID3D12CommandQueue* m_CommandQueues[NR_OF_QUEUES]{ nullptr ,nullptr ,nullptr };
	ID3D12CommandAllocator* m_CommandAllocators[NR_OF_QUEUES]{ nullptr };
	ID3D12GraphicsCommandList4* m_CommandLists[NR_OF_QUEUES]{ nullptr };

	std::string m_QueueNames[NR_OF_QUEUES];

	//Main Direct
	ID3D12CommandQueue* m_MainCommandQueue{ nullptr };
	ID3D12CommandAllocator* m_MainCommandAllocator{ nullptr };
	ID3D12GraphicsCommandList4* m_MainCommandList{ nullptr };

	bool CreateResource(ResourceDesc& ResourceDescriptor, ID3D12Resource*& Resource) noexcept;
	void CopyResource(const CommandPointers& Command, ID3D12Resource*& SrcResource, D3D12_RESOURCE_STATES CurrentStateOfSrcResource, ID3D12Resource*& DestResource, D3D12_RESOURCE_STATES CurrentStateOfDestResource, std::string copyDescription) noexcept;
	bool AddDataToResource(const ResourceDesc& ResourceDescriptor, ID3D12Resource*& Resource);
	bool ReadDataFromResource(const uint64_t& DataSize, ID3D12Resource*& Resource);

	bool m_ApplicationRunning = false;


	//Fence
	ID3D12Fence* m_Fence;
	int m_FenceValue{ 0 };


	UINT64 m_dataSize{ 0 };
	UINT64 m_NrOfElements{ 0 };
	int m_SizeOfOneElement{ sizeof(UINT64) };


	DXGI_QUERY_VIDEO_MEMORY_INFO  m_VideoMemoryInfo;
	DXGI_QUERY_VIDEO_MEMORY_INFO m_SystemMemoryInfo;


	std::vector<TimeStamp> m_TimeStamps;
	std::vector<float> m_TimeStampsMemCopy;
	
	std::string m_GPU_Name{""};


public:
	BufferRenderer() noexcept;
	~BufferRenderer() noexcept;
	BufferRenderer(const BufferRenderer&) = delete;
	void operator = (const BufferRenderer&) = delete;


	void InitalizeWindowAndDevice(const WindowsProperties& winProp) noexcept;
	void InitalizeRenderer() noexcept;

	void Run();
private:
	void RunTest(CommandPointers& ComPtrs, float sleepTime);
	void ShutDown();
	void HandleInputEvents();
	void HandleKeyPressedEvent();

	void CreateFence();
	void CreateDevice();
	void CreateCommandInterfaces();


	void CreateData(std::vector<UINT64>* vector, UINT64 nrOfBytes);
	UINT8 RandNumber(UINT8 min, UINT8 max);
	bool CompareData(std::vector<UINT64>* vector1, std::vector<UINT64>* vector2);
	
	void Wait(float milliseconds);
private:
	template<class Interface>
	inline void SafeRelease(Interface** ppInterfaceToRelease)
	{
		if (*ppInterfaceToRelease != NULL)
		{
			(*ppInterfaceToRelease)->Release();
			(*ppInterfaceToRelease) = NULL;
		}
	}


};





#endif // !BUFFER_RENDERER
