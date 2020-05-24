#include "BufferRenderer.h"


BufferRenderer::BufferRenderer() noexcept
{
}

BufferRenderer::~BufferRenderer() noexcept
{
}

void BufferRenderer::InitalizeWindowAndDevice(const WindowsProperties& winProp) noexcept
{
	m_WindowProperties = winProp;
	CreateDevice();

}

void BufferRenderer::InitalizeRenderer() noexcept
{

	CreateCommandInterfaces();
	CreateFence();


}

void BufferRenderer::Run()
{

#pragma region Collect Data

	m_Device->SetStablePowerState(true);

	UINT64 maxTestSize = 2147483648;
	//maxTestSize /= 2;
	//maxTestSize *= 20;
	UINT64 sizeIncrease = 67108864;//131072000; //65536000
	float nrOfIncreases = (float)maxTestSize / (float)sizeIncrease;
	int nrOfValidationTests = 10;
	float ExpectedTime = 0;

	float avgDiff = 0.0f;

	float sleepTimeNormal = 1000; //Milliseconds
	float sleepTimeSmallSizes = 1000;
	float sleepTime = sleepTimeNormal;
	UINT64 sizeToSwitchSleepTime = 268435456;

	//Sleep(10000);
#pragma region Memcopy
	std::cout << "MEMCOPY TEST " << std::endl;

	m_TimeStampsMemCopy.resize(nrOfIncreases);

	for (int dataI = 0; dataI < nrOfIncreases; dataI++)
	{
		m_dataSize += sizeIncrease;
		m_NrOfElements = m_dataSize;
		m_NrOfElements /= 8;


		for (int i = 0; i < nrOfValidationTests; i++)
		{

			UINT64* arrSource = new UINT64[m_NrOfElements];
			UINT64* arrDestination = new UINT64[m_NrOfElements];
			UINT64 data = 0;
			double endTime = 0;


			for (UINT64 i = 0; i < m_NrOfElements; i++)
			{
				arrSource[i] = i;
				arrDestination[i] = 0;
			}


			auto startTime = std::chrono::high_resolution_clock::now();

			memcpy(arrSource, arrDestination, m_dataSize);

			endTime = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTime)).count();
			m_TimeStampsMemCopy.at((m_dataSize / sizeIncrease) - 1) += endTime;



			for (size_t i = 0; i < m_NrOfElements; i++)
			{

				memcpy(&data, arrDestination, m_SizeOfOneElement);

				if (data != i)
				{
					//Error
					assert(0);
				}
			}


			delete[] arrSource;
			delete[] arrDestination;
			Wait(500);
		}
		

		std::cout << "Test: " << dataI + 1 << "/" << nrOfIncreases << " Size: " << m_dataSize <<  std::endl;
	}

	for (int i = 0; i < m_TimeStampsMemCopy.size(); i++)
	{
		m_TimeStampsMemCopy.at(i) /= (nrOfValidationTests * 1e6);					//Save data from this vector to file (ok)
		std::cout << m_TimeStampsMemCopy.at(i) << std::endl;
	}

#pragma endregion

#pragma region GPUTests
	std::cout << "GPU TEST " << std::endl;

	m_dataSize = 0;
	auto startTimeTotal = std::chrono::high_resolution_clock::now();

	for (int dataI = 0; dataI < nrOfIncreases; dataI++)
	{


		auto startTimeData = std::chrono::high_resolution_clock::now();

		//Set up the data
		m_dataSize += sizeIncrease;

		if (m_dataSize < sizeToSwitchSleepTime)
		{
			sleepTime = sleepTimeSmallSizes;
		}
		else
		{
			sleepTime = sleepTimeNormal;
		}

		m_NrOfElements = m_dataSize;
		m_NrOfElements /= 8;

		

		auto startTimeOneTest = std::chrono::high_resolution_clock::now();

		for (int queueI = 0; queueI < 1; queueI++)
		{
			// Choose which List, Allocator and Queue to use
			CommandPointers ComPtrs;
			ComPtrs.Allocator = m_CommandAllocators[queueI];
			ComPtrs.List = m_CommandLists[queueI];
			ComPtrs.Queue = m_CommandQueues[queueI];
			ComPtrs.QueueName = m_QueueNames[queueI];
			ComPtrs.CopySize = m_dataSize;

			for (size_t i = 0; i < nrOfValidationTests; i++)
			{
				RunTest(ComPtrs, sleepTime);
			}
		}
		double endTimeOneTest = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTimeOneTest)).count();

		if (ExpectedTime == 0 && sleepTime == sleepTimeNormal)
		{
			for (size_t i = 1; i <= (nrOfIncreases); i++)
			{
				ExpectedTime += endTimeOneTest / 1e9 + 0.1475 * nrOfIncreases;
			}
			std::cout << "Expected time " << (ExpectedTime) << "s" << std::endl;



		}


		double endTimeData = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTimeData)).count();
		std::cout << "Test: " << dataI + 1 << "/" << nrOfIncreases << " Size: " << m_dataSize << " Time: " << endTimeData / 1e9 << "s" << std::endl;

		avgDiff = endTimeData / 1e9;

	}
	double endTimeTotal = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTimeTotal)).count();
	std::cout << "Total time: " << endTimeTotal / 1e9 << "s" << std::endl;

	std::cout << "Average time: " << endTimeTotal / (1e9 * nrOfIncreases * nrOfValidationTests) << "s" << std::endl;
#pragma endregion
	
	std::cout << "Saving to file... " << std::endl;


	




#pragma endregion

#pragma region SaveToXML



#pragma region FolderHandling
	//Creating a save folder for the tests

	std::string finalPath{ "" };

	//Create Completed Dir
	if (!std::filesystem::exists(TEST_FOLDER_NAME))
	{
		if (_mkdir(std::string(TEST_FOLDER_NAME).c_str()) != 0)
		{
			std::cout << "Could not create folder: " << TEST_FOLDER_NAME << std::endl;
		}
		else
		{
			finalPath += std::string(TEST_FOLDER_NAME) + "/";
		}
	}
	else
	{
		finalPath += std::string(TEST_FOLDER_NAME) + "/";
	}


	std::string tmpGPUfolderName = finalPath + m_GPU_Name;

	//Create folder for GPU inside test folder
	if (!std::filesystem::exists(tmpGPUfolderName))
	{
		if (_mkdir(tmpGPUfolderName.c_str()) != 0)
		{
			std::cout << "Could not create folder: " << m_GPU_Name << "\t Inside test folder: " << TEST_FOLDER_NAME << "." << std::endl;
		}
		else
		{
			finalPath += m_GPU_Name + "/";
		}
	}
	else
	{
		finalPath += m_GPU_Name + "/";
	}


#pragma endregion


#pragma region Structure XML

	//Correcting to acceptable XML characters

	std::string saveFilePath{ "" };

	// Time Stamp
	auto end = std::chrono::system_clock::now();
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	char* charptr = std::ctime(&end_time);


	std::string tmpStr{ "" };
	int counter = 0;
	int timeIndex = -1;
	while (charptr[counter] != '\0')
	{
		tmpStr += charptr[counter];


		if (charptr[counter] == ':' && timeIndex == -1)
		{
			timeIndex = counter - 2;
		}

		counter++;
	}


	std::string tmpTimeStr{ "H" };
	char ms[2]{ 'M','S' };
	int msCount = 0;

	for (int i = timeIndex; i < (timeIndex + 8); i++)
	{
		if (charptr[i] != ':')
		{
			tmpTimeStr += charptr[i];
		}
		else
		{
			char tt = ms[msCount];
			tmpTimeStr += "_";
			tmpTimeStr += ms[msCount++];
		}
	}



#pragma region Sort and save TimeStamps to XML





	tinyxml2::XMLDocument pDocSorted;
	pDocSorted.NewDeclaration(NULL);


	std::string XMLstrName = m_GPU_Name;

	for (int i = 0; i < XMLstrName.size(); i++)
	{
		if (XMLstrName[i] == (char)' ')
		{
			XMLstrName.replace(i, 1, "_");
		}
	}


	tinyxml2::XMLNode* pRootNode = pDocSorted.NewElement(XMLstrName.c_str());
	pDocSorted.InsertFirstChild(pRootNode);


	std::unordered_map<UINT64, std::vector<TimeStamp>> DataSizeOrder;
	std::unordered_map<std::string, std::vector<TimeStamp>> HeapOrder;
	std::unordered_map<std::string, std::vector<TimeStamp>> QueueOrder;

	for (int i = 0; i < m_TimeStamps.size(); i++)
	{
		QueueOrder[m_TimeStamps[i].Queue].emplace_back(m_TimeStamps[i]);
		HeapOrder[m_TimeStamps[i].HeapFromTo].emplace_back(m_TimeStamps[i]);
		DataSizeOrder[m_TimeStamps[i].DataSize].emplace_back(m_TimeStamps[i]);
	}


#pragma region Average Queue Order
	std::unordered_map<std::string, std::vector<TimeStamp>>::iterator Queue_It = QueueOrder.begin();


	tinyxml2::XMLElement* pQueueRoot = pDocSorted.NewElement("Average_Time_Queue");
	pRootNode->InsertEndChild(pQueueRoot);


	while (Queue_It != QueueOrder.end())
	{
		double average = 0.0;
		for (int i = 0; i < Queue_It->second.size(); i++)
		{
			average += Queue_It->second.at(i).Time;
		}
		average /= Queue_It->second.size();


		tinyxml2::XMLElement* pQueue = pDocSorted.NewElement(Queue_It->first.c_str());
		pQueue->SetText(average);

		pQueueRoot->InsertEndChild(pQueue);
		Queue_It++;
	}

#pragma endregion

#pragma region Average Size Order
	std::unordered_map<UINT64, std::vector<TimeStamp>>::iterator Size_It = DataSizeOrder.begin();

	tinyxml2::XMLElement* pSizeRoot = pDocSorted.NewElement("Average_Time_DataSize");
	pRootNode->InsertEndChild(pSizeRoot);

	avgDiff = avgDiff / nrOfIncreases;
	std::cout << "Average diff " << avgDiff << "s" << std::endl;

	//Queue Sort
	std::unordered_map<std::string, std::vector<TimeStamp>> QueueSortMap;
	std::unordered_map<std::string, std::vector<TimeStamp>>::iterator QueueSortMap_It;

	//HeapType, AverageVal
	std::unordered_map<std::string, std::vector<double>> HeapSortMap;
	std::unordered_map<std::string, std::vector<double>>::iterator HeapSortMap_It;


	while (Size_It != DataSizeOrder.end())
	{
		QueueSortMap.clear();


		//From Queue
		for (int i = 0; i < Size_It->second.size(); i++)
		{
			QueueSortMap[Size_It->second.at(i).Queue.c_str()].emplace_back(Size_It->second.at(i));
		}

		QueueSortMap_It = QueueSortMap.begin();

		while (QueueSortMap_It != QueueSortMap.end())
		{
			HeapSortMap.clear();
			//From Heap
			for (int i = 0; i < QueueSortMap_It->second.size(); i++)
			{
				HeapSortMap[QueueSortMap_It->second.at(i).HeapFromTo.c_str()].emplace_back(QueueSortMap_It->second.at(i).Time);
			}

			HeapSortMap_It = HeapSortMap.begin();

			while (HeapSortMap_It != HeapSortMap.end())
			{
				double average = 0.0;
				for (int i = 0; i < HeapSortMap_It->second.size(); i++)
				{
					average += HeapSortMap_It->second.at(i);
				}
				average /= HeapSortMap_It->second.size();



				tinyxml2::XMLElement* stamp = pDocSorted.NewElement("TimeStamp");

				tinyxml2::XMLElement* Time = pDocSorted.NewElement("Average_Time");
				Time->SetText(average);
				tinyxml2::XMLElement* Queue = pDocSorted.NewElement("Queue");
				Queue->SetText(QueueSortMap_It->first.c_str());
				tinyxml2::XMLElement* Heap = pDocSorted.NewElement("Copy_From-->To");
				Heap->SetText(HeapSortMap_It->first.c_str());
				tinyxml2::XMLElement* Size = pDocSorted.NewElement("Data_Size");
				Size->SetText(Size_It->first);

				stamp->InsertEndChild(Queue);
				stamp->InsertEndChild(Heap);
				stamp->InsertEndChild(Size);
				stamp->InsertEndChild(Time);

				pRootNode->InsertEndChild(stamp);




				HeapSortMap_It++;
			}
			QueueSortMap_It++;
		}
		Size_It++;
	}




	std::vector<TimeStamp> AverageTimes_DataSize;





#pragma endregion



	saveFilePath = finalPath + m_GPU_Name + " " + tmpTimeStr + " Sorted data.xml";
	if (tinyxml2::XML_SUCCESS != pDocSorted.SaveFile(saveFilePath.c_str()))
	{
		//Error
	}

#pragma endregion

#pragma region RAW DATA XML


	tinyxml2::XMLDocument pDocR;
	pDocR.NewDeclaration(NULL);
	tinyxml2::XMLNode* pRootNodeRAW = pDocR.NewElement(XMLstrName.c_str());
	pDocR.InsertFirstChild(pRootNodeRAW);


	for (int i = 0; i < m_TimeStamps.size(); i++)
	{
		tinyxml2::XMLElement* stamp = pDocR.NewElement("TimeStamp");

		tinyxml2::XMLElement* Time = pDocR.NewElement("Time");
		Time->SetText(m_TimeStamps[i].Time);
		tinyxml2::XMLElement* Queue = pDocR.NewElement("Queue");
		Queue->SetText(m_TimeStamps[i].Queue.c_str());
		tinyxml2::XMLElement* Heap = pDocR.NewElement("Copy_From-->To");
		Heap->SetText(m_TimeStamps[i].HeapFromTo.c_str());
		tinyxml2::XMLElement* Size = pDocR.NewElement("Data_Size");
		Size->SetText(m_TimeStamps[i].DataSize);

		stamp->InsertEndChild(Queue);
		stamp->InsertEndChild(Heap);
		stamp->InsertEndChild(Size);
		stamp->InsertEndChild(Time);

		pRootNodeRAW->InsertEndChild(stamp);
	}

#pragma endregion


	saveFilePath = finalPath + m_GPU_Name + " " + tmpTimeStr + " Raw.xml";
	if (tinyxml2::XML_SUCCESS != pDocR.SaveFile(saveFilePath.c_str()))
	{
		//Error
	}



#pragma endregion

#pragma endregion


#pragma region MemCpyData

	tinyxml2::XMLDocument pDocMemcpy;
	pDocMemcpy.NewDeclaration(NULL);

	tinyxml2::XMLNode* pRootNodeMem = pDocMemcpy.NewElement("Cpu_Memory_Copy_Timestamps");
	pDocMemcpy.InsertFirstChild(pRootNodeMem);


	tinyxml2::XMLElement* pMemRootElement = pDocMemcpy.NewElement("Increase_size");
	pMemRootElement->SetText(sizeIncrease);

	pRootNodeMem->InsertEndChild(pMemRootElement);




	for (int i = 0; i < m_TimeStampsMemCopy.size(); i++)
	{

		tinyxml2::XMLElement* MemNode = pDocMemcpy.NewElement("Copy_Time");
		MemNode->SetText(m_TimeStampsMemCopy.at(i));

		pMemRootElement->InsertEndChild(MemNode);
	}


	std::string memcpyFilePath = finalPath + m_GPU_Name + " " + tmpTimeStr + " MemCpy.xml";
	if (tinyxml2::XML_SUCCESS != pDocMemcpy.SaveFile(memcpyFilePath.c_str()))
	{
		//Error
	}

#pragma endregion


	std::cout << "Done " << std::endl;

	m_ApplicationRunning = true;
	while (m_ApplicationRunning)
	{
		HandleInputEvents();
	}
	ShutDown();
}

void BufferRenderer::RunTest(CommandPointers& ComPtrs, float sleepTime)
{
	//auto startTime = std::chrono::high_resolution_clock::now();
	UINT64 expectedSystemMem = 0;
	UINT64 expectedVideoMem = 0;;
	UINT64 currentSystemMem = 0;
	UINT64 currentVideoMem = 0;


#pragma region Test1


	//Descriptors for resource creation

	ResourceDesc UploadDesc;
	UploadDesc.DataSize = m_dataSize;
	UploadDesc.HeapFinalState = D3D12_RESOURCE_STATE_GENERIC_READ;
	UploadDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	UploadDesc.Name = "UploadType1";

	ResourceDesc DefaultDesc;
	DefaultDesc.DataSize = UploadDesc.DataSize;
	DefaultDesc.HeapFinalState = D3D12_RESOURCE_STATE_GENERIC_READ;
	DefaultDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	DefaultDesc.Name = "DefaultType1";

	ResourceDesc ReadbackDesc;
	ReadbackDesc.DataSize = UploadDesc.DataSize;
	ReadbackDesc.HeapFinalState = D3D12_RESOURCE_STATE_COPY_DEST;
	ReadbackDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
	ReadbackDesc.Name = "ReadbackType1";


	//Resource Pointers Creation

	ID3D12Resource* UploadResource;
	ID3D12Resource* ReadbackResource;
	ID3D12Resource* DefaultResource;
	ID3D12Resource* DefaultResourceDest;



	if (!CreateResource(UploadDesc, UploadResource))
	{
		assert(0);
	}

	if (!CreateResource(DefaultDesc, DefaultResourceDest))
	{
		assert(0);
	}

	if (!CreateResource(DefaultDesc, DefaultResource))
	{
		assert(0);
	}

	if (!CreateResource(ReadbackDesc, ReadbackResource))
	{
		assert(0);
	}




	// Add data to upload resource
	if (!AddDataToResource(UploadDesc, UploadResource))
	{
		//Error
		assert(0);
	}

	Wait(1000);

	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
	currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
	currentVideoMem = m_VideoMemoryInfo.CurrentUsage;

	// Copy data between the resources

	// Upload --> Default
	CopyResource(ComPtrs, UploadResource, D3D12_RESOURCE_STATE_GENERIC_READ, DefaultResourceDest, D3D12_RESOURCE_STATE_GENERIC_READ, "Upload-->Default");
	//Default --> Default
	CopyResource(ComPtrs, DefaultResourceDest, D3D12_RESOURCE_STATE_GENERIC_READ, DefaultResource, D3D12_RESOURCE_STATE_GENERIC_READ, "Default-->Default");
	// Default --> Readback
	CopyResource(ComPtrs, DefaultResource, D3D12_RESOURCE_STATE_GENERIC_READ, ReadbackResource, D3D12_RESOURCE_STATE_COPY_DEST, "Default-->Readback");


	// Read data from last copy resource for validation
	ReadDataFromResource(m_NrOfElements, ReadbackResource);




#pragma endregion

#pragma region CleanUp1



	UploadResource->Release();
	ReadbackResource->Release();
	DefaultResource->Release();
	DefaultResourceDest->Release();

	/*m_MainCommandAllocator->Reset();
	m_MainCommandList->Reset(m_MainCommandAllocator, nullptr);
	m_MainCommandList->Close();
	ID3D12CommandList* list[] = { m_MainCommandList };
	m_MainCommandQueue->ExecuteCommandLists(ARRAYSIZE(list), list);*/


	expectedSystemMem = currentSystemMem - m_dataSize * 2;
	expectedVideoMem = currentVideoMem - m_dataSize * 2;

	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
	currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
	currentVideoMem = m_VideoMemoryInfo.CurrentUsage;


	while (currentSystemMem * 0.99 > expectedSystemMem || currentVideoMem * 0.85 > expectedVideoMem)
	{
		m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
		m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
		currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
		currentVideoMem = m_VideoMemoryInfo.CurrentUsage;
	}


	//Sleep(sleepTime);
	Wait(sleepTime);


#pragma endregion

#pragma region Test2


	// Create Resources

	ID3D12Resource* UploadResource2;
	ID3D12Resource* ReadbackResource2;




	if (!CreateResource(UploadDesc, UploadResource2))
	{
		//Error
		assert(0);
	}

	if (!CreateResource(ReadbackDesc, ReadbackResource2))
	{
		//Error
		assert(0);
	}
	//Sleep(sleepTime);
	Wait(1000);


	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
	currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
	currentVideoMem = m_VideoMemoryInfo.CurrentUsage;

	if (!AddDataToResource(UploadDesc, UploadResource2))
	{
		//Error
		assert(0);
	}

	// Upload --> Readback
	CopyResource(ComPtrs, UploadResource2, D3D12_RESOURCE_STATE_GENERIC_READ, ReadbackResource2, D3D12_RESOURCE_STATE_COPY_DEST, "Upload-->Readback");



	ReadDataFromResource(m_NrOfElements, ReadbackResource2);





#pragma endregion



#pragma region CleanUp2


	UploadResource2->Release();
	ReadbackResource2->Release();

	/*m_MainCommandAllocator->Reset();
	m_MainCommandList->Reset(m_MainCommandAllocator, nullptr);
	m_MainCommandList->Close();
	ID3D12CommandList* list2[] = { m_MainCommandList };
	m_MainCommandQueue->ExecuteCommandLists(ARRAYSIZE(list2), list2);*/


	expectedSystemMem = currentSystemMem - m_dataSize * 2;
	expectedVideoMem = currentVideoMem - m_dataSize * 0;

	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
	m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
	currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
	currentVideoMem = m_VideoMemoryInfo.CurrentUsage;


	while (currentSystemMem * 0.99 > expectedSystemMem || currentVideoMem * 0.99 > expectedVideoMem)
	{
		m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_VideoMemoryInfo);
		m_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_SystemMemoryInfo);
		currentSystemMem = m_SystemMemoryInfo.CurrentUsage;
		currentVideoMem = m_VideoMemoryInfo.CurrentUsage;
	}


	//Sleep(sleepTime);
	Wait(sleepTime);

#pragma endregion

	//double endTime = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTime)).count();
	//std::cout << "Time: " << endTime / 1e9 << "s" << std::endl;
}

void BufferRenderer::ShutDown()
{
	SafeRelease(&m_Device);
	SafeRelease(&m_Adapter);
	SafeRelease(&m_Fence);
	SafeRelease(&m_MainCommandQueue);
	SafeRelease(&m_MainCommandAllocator);
	SafeRelease(&m_MainCommandList);

	for (size_t i = 0; i < 3; i++)
	{
		SafeRelease(&m_CommandQueues[i]);
		SafeRelease(&m_CommandAllocators[i]);
		SafeRelease(&m_CommandLists[i]);
	}

	SDL_DestroyWindow(m_SDLWindow);
}

void BufferRenderer::HandleInputEvents()
{
	while (SDL_PollEvent(&m_Events))
	{
		switch (m_Events.type)
		{
		case SDL_QUIT:
			m_ApplicationRunning = false;
			break;
		}

	}
}

void BufferRenderer::HandleKeyPressedEvent()
{
	switch (m_Events.key.keysym.sym)
	{
	case SDLK_ESCAPE:
		m_ApplicationRunning = false;
		break;
	}
}

void BufferRenderer::CreateFence()
{
	m_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
}

void BufferRenderer::CreateDevice()
{
#ifdef _DEBUG
	// The Debug layer has to be enabled before the device is created, otherwise the device will be removed.
	ID3D12Debug* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(&debugController);
#endif // _DEBUG


	IDXGIFactory7* factory = nullptr;

	CreateDXGIFactory(IID_PPV_ARGS(&factory));


	for (UINT i = 0;; i++)
	{
		m_Adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters(i, (IDXGIAdapter**)&m_Adapter))
		{
			break; //No more adapters to enumerate.
		}



		if (SUCCEEDED(D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device5), nullptr)))
		{
			break;
		}

	}

	if (m_Adapter)
	{
		HRESULT hr = S_OK;
		//Does not work with device 6
		if (SUCCEEDED(hr = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device))))
		{

		}
	}
	else
	{
		factory->EnumWarpAdapter(IID_PPV_ARGS(&m_Adapter));
		D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};

	m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData));

	DXGI_ADAPTER_DESC desc;
	m_Adapter->GetDesc(&desc);

	std::wstring tempName = desc.Description;
	m_GPU_Name = std::string(tempName.begin(), tempName.end());


	SafeRelease(&factory);
}

void BufferRenderer::CreateCommandInterfaces()
{
	D3D12_COMMAND_LIST_TYPE listTypes[NR_OF_QUEUES]{
		D3D12_COMMAND_LIST_TYPE_DIRECT ,
		D3D12_COMMAND_LIST_TYPE_COPY ,
		D3D12_COMMAND_LIST_TYPE_COMPUTE };

	m_QueueNames[0] = "Direct";
	m_QueueNames[1] = "Copy";
	m_QueueNames[2] = "Compute";


	HRESULT hr = S_OK;

	for (int i = 0; i < NR_OF_QUEUES; i++)
	{

		D3D12_COMMAND_QUEUE_DESC cqdDirect = {};
		cqdDirect.Type = listTypes[i];


		if (FAILED(hr = m_Device->CreateCommandQueue(&cqdDirect, IID_PPV_ARGS(&m_CommandQueues[i]))))
		{
			//ERROR
			assert(0);
		}

		if (FAILED(hr = m_Device->CreateCommandAllocator(listTypes[i], IID_PPV_ARGS(&m_CommandAllocators[i]))))
		{
			//ERROR
			assert(0);
		}

		if (FAILED(hr = m_Device->CreateCommandList(
			0,
			listTypes[i],
			m_CommandAllocators[i],
			nullptr,
			IID_PPV_ARGS(&m_CommandLists[i]))))
		{
			//ERROR
			assert(0);
		}

		m_CommandLists[i]->Close();
	}


	//Main Direct: Queue, List, Allocator

	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//assert(testPtr != nullptr, "msg");


	//Queue
	if (FAILED(hr = m_Device->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_MainCommandQueue))))
	{
		//	throw("Could not create Main CommandQueue");
			//ERROR
		assert(0);
	}

	//Allocator
	if (FAILED(m_Device->CreateCommandAllocator(cqd.Type, IID_PPV_ARGS(&m_MainCommandAllocator))))
	{
		assert(0);
		//throw("Could not create Main CommandAllocator");
		//ERROR

	}

	//List
	if (FAILED(m_Device->CreateCommandList(
		0,
		cqd.Type,
		m_MainCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&m_MainCommandList))))
	{
		//ERROR
		//throw("Could not create Main CommandList");
		assert(0);
	}

	m_MainCommandList->Close();

}

void BufferRenderer::CreateData(std::vector<UINT64>* vector, UINT64 nrOfBytes)
{
	for (size_t i = 0; i < nrOfBytes; i++)
	{
		vector->emplace_back(i);
	}
}

UINT8 BufferRenderer::RandNumber(UINT8 min, UINT8 max)
{
	return std::rand() % (min - max + 1) + min;
}

bool BufferRenderer::CompareData(std::vector<UINT64>* vector1, std::vector<UINT64>* vector2)
{
	for (size_t i = 0; i < vector1->size(); i++)
	{
		if (vector1->at(i) != vector2->at(i))
		{
			return false;
		}
	}

	return true;
}

void BufferRenderer::Wait(float milliseconds)
{
	auto startTime = std::chrono::high_resolution_clock::now();
	double endTime = 0;
	while (endTime < milliseconds)
	{
		endTime = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::high_resolution_clock::now() - startTime)).count();

	}
}

bool BufferRenderer::CreateResource(ResourceDesc& ResourceDescriptor, ID3D12Resource*& Resource) noexcept
{
	bool returnValue = false;

	UINT cbSizeAligned = (ResourceDescriptor.DataSize + 255) & ~255;

	D3D12_RESOURCE_DESC finalResourceDesc{};
	finalResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	finalResourceDesc.Width = cbSizeAligned;
	finalResourceDesc.Height = 1;
	finalResourceDesc.DepthOrArraySize = 1;
	finalResourceDesc.MipLevels = 1;
	finalResourceDesc.SampleDesc.Count = 1;
	finalResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES finalHeapProperties = {};
	finalHeapProperties.Type = ResourceDescriptor.HeapType;
	finalHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	finalHeapProperties.CreationNodeMask = 1;
	finalHeapProperties.VisibleNodeMask = 1;
	finalHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;


	D3D12_RESOURCE_STATES ResourceState = ResourceDescriptor.HeapFinalState;


	if (SUCCEEDED(m_Device->CreateCommittedResource(
		&finalHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&finalResourceDesc,
		ResourceState,
		nullptr,
		IID_PPV_ARGS(&Resource))))
	{
		returnValue = true;

		std::wstring tempwString = std::wstring(ResourceDescriptor.Name.begin(), ResourceDescriptor.Name.end());
		Resource->SetName(tempwString.c_str());
	}
	D3D12_RESOURCE_ALLOCATION_INFO info = m_Device->GetResourceAllocationInfo(0, 1, &finalResourceDesc);

	return returnValue;

}

void BufferRenderer::CopyResource(const CommandPointers& Command, ID3D12Resource*& SrcResource, D3D12_RESOURCE_STATES CurrentStateOfSrcResource, ID3D12Resource*& DestResource, D3D12_RESOURCE_STATES CurrentStateOfDestResource, std::string copyDescription) noexcept
{

#pragma region ChangeStateOfDeafultTypes

	//Check destination resource type
	D3D12_HEAP_PROPERTIES SrcProps{};
	SrcResource->GetHeapProperties(&SrcProps, nullptr);


	D3D12_HEAP_PROPERTIES DestProps{};
	DestResource->GetHeapProperties(&DestProps, nullptr);




	// Change the new resource state to final && Change back the old resource state from copy source
	// (Not the cleanest solution)
	if (DestProps.Type == D3D12_HEAP_TYPE_DEFAULT || SrcProps.Type == D3D12_HEAP_TYPE_DEFAULT)
	{

		m_MainCommandAllocator->Reset();
		m_MainCommandList->Reset(m_MainCommandAllocator, nullptr);

		D3D12_RESOURCE_BARRIER Barriers[2];
		uint8_t NumberOfBarriers = 0;

		if (DestProps.Type == D3D12_HEAP_TYPE_DEFAULT)
		{
			D3D12_RESOURCE_BARRIER barrierDesc1 = {};

			barrierDesc1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc1.Transition.pResource = DestResource;
			barrierDesc1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrierDesc1.Transition.StateBefore = CurrentStateOfDestResource;
			barrierDesc1.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

			Barriers[NumberOfBarriers] = barrierDesc1;
			NumberOfBarriers++;
		}


		if (SrcProps.Type == D3D12_HEAP_TYPE_DEFAULT)
		{
			D3D12_RESOURCE_BARRIER barrierDesc1 = {};

			barrierDesc1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc1.Transition.pResource = SrcResource;
			barrierDesc1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrierDesc1.Transition.StateBefore = CurrentStateOfSrcResource;
			barrierDesc1.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

			Barriers[NumberOfBarriers] = barrierDesc1;
			NumberOfBarriers++;
		}


		m_MainCommandList->ResourceBarrier(NumberOfBarriers, Barriers);



		m_MainCommandList->Close();
		ID3D12CommandList* list[] = { m_MainCommandList };
		m_MainCommandQueue->ExecuteCommandLists(ARRAYSIZE(list), list);

		m_FenceValue++;
		m_MainCommandQueue->Signal(m_Fence, m_FenceValue);

		while (m_Fence->GetCompletedValue() != m_FenceValue)
		{

		}

	}

#pragma endregion

#pragma region Copy

	Command.Allocator->Reset();
	Command.List->Reset(Command.Allocator, nullptr);
	Command.List->CopyResource(DestResource, SrcResource);
	Command.List->Close();
	ID3D12CommandList* executeLists[] = { Command.List };
	Command.Queue->ExecuteCommandLists(1, executeLists);
	auto startTime = std::chrono::high_resolution_clock::now();

	m_FenceValue++;
	Command.Queue->Signal(m_Fence, m_FenceValue);

	while (m_Fence->GetCompletedValue() != m_FenceValue)
	{

	}

	double endTime = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - startTime)).count();
	double timeToFilevalue = endTime / 1e6;

	TimeStamp tmpStamp;

	tmpStamp.Time = timeToFilevalue;
	tmpStamp.Queue = Command.QueueName;
	tmpStamp.DataSize = Command.CopySize;
	tmpStamp.HeapFromTo = copyDescription;

	m_TimeStamps.emplace_back(tmpStamp);

#pragma endregion

#pragma region ChangeBackStateOfDeafultTypes


	// Change the new resource state to final && Change back the old resource state from copy source
	// (Not the cleanest solution)
	if (DestProps.Type == D3D12_HEAP_TYPE_DEFAULT || SrcProps.Type == D3D12_HEAP_TYPE_DEFAULT)
	{

		m_MainCommandAllocator->Reset();
		m_MainCommandList->Reset(m_MainCommandAllocator, nullptr);

		D3D12_RESOURCE_BARRIER Barriers[2];
		uint8_t NumberOfBarriers = 0;

		if (DestProps.Type == D3D12_HEAP_TYPE_DEFAULT)
		{
			D3D12_RESOURCE_BARRIER barrierDesc1 = {};

			barrierDesc1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc1.Transition.pResource = DestResource;
			barrierDesc1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrierDesc1.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrierDesc1.Transition.StateAfter = CurrentStateOfDestResource;

			Barriers[NumberOfBarriers] = barrierDesc1;
			NumberOfBarriers++;
		}


		if (SrcProps.Type == D3D12_HEAP_TYPE_DEFAULT)
		{
			D3D12_RESOURCE_BARRIER barrierDesc1 = {};

			barrierDesc1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc1.Transition.pResource = SrcResource;
			barrierDesc1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrierDesc1.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			barrierDesc1.Transition.StateAfter = CurrentStateOfSrcResource;

			Barriers[NumberOfBarriers] = barrierDesc1;
			NumberOfBarriers++;
		}


		m_MainCommandList->ResourceBarrier(NumberOfBarriers, Barriers);



		m_MainCommandList->Close();
		ID3D12CommandList* list[] = { m_MainCommandList };
		m_MainCommandQueue->ExecuteCommandLists(ARRAYSIZE(list), list);

		m_FenceValue++;
		m_MainCommandQueue->Signal(m_Fence, m_FenceValue);

		while (m_Fence->GetCompletedValue() != m_FenceValue)
		{

		}

	}

#pragma endregion

}

bool BufferRenderer::AddDataToResource(const ResourceDesc& ResourceDescriptor, ID3D12Resource*& Resource)
{

	bool returnValue = false;

	void* mappedMem = nullptr;
	D3D12_RANGE readRange = { 0, 0 };

	UINT64 data = 0;

	if (SUCCEEDED(Resource->Map(0, &readRange, &mappedMem)))
	{
		for (UINT64 i = 0; i < m_NrOfElements; i++)
		{
			data = i;

			memcpy(mappedMem, &data, m_SizeOfOneElement);
			mappedMem = (char*)mappedMem + m_SizeOfOneElement;
		}


		D3D12_RANGE writeRange = { 0, ResourceDescriptor.DataSize };
		Resource->Unmap(0, &writeRange);

		returnValue = true;
	}

	return returnValue;
}

bool BufferRenderer::ReadDataFromResource(const uint64_t& DataSize, ID3D12Resource*& Resource)
{
	bool returnValue = false;
	void* mappedMem = nullptr;
	D3D12_RANGE readRange = { 0, DataSize };
	UINT64 data = 0;

	if (SUCCEEDED(Resource->Map(0, &readRange, &mappedMem)))
	{
		for (size_t i = 0; i < m_NrOfElements; i++)
		{

			memcpy(&data, mappedMem, m_SizeOfOneElement);
			mappedMem = (char*)mappedMem + m_SizeOfOneElement;

			if (data != i)
			{
				//Error
				assert(0);
			}
		}
		D3D12_RANGE writeRange = { 0, 0 };
		Resource->Unmap(0, &writeRange);

		returnValue = true;
	}
	return returnValue;
}