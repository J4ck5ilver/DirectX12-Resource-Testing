
#include<ios>
#include<iostream>

#include"Renderer/BufferRenderer.h"

#include <vector>

int main() 
{

	//int nrOfMegaBytes = 1024 * 4;
	////const int size = 250000 * nrOfMegaBytes ;
	//const int size = 0xffffffff / 4;

	////double data[size];

	//std::vector<int> data;
	//data.reserve(size);
	//for (size_t i = 0; i < size; i++)
	//{
	//	data.emplace_back(i);
	//}

	//

	//int64_t sizeInBytes = sizeof(int)* (int64_t)data.size();
	//float sizeInBytesKilo = sizeInBytes /  (float)0x400;
	//float sizeInBytesMega = sizeInBytes / (float)0x100000;
	//float sizeInBytesGiga = sizeInBytes / (float)0x40000000;

	//std::cout << "Size in bytes:" << sizeInBytes << std::endl;
	//std::cout << "Size in Kilobytes:" << sizeInBytesKilo << std::endl;
	//std::cout << "Size in Megabytes:" << sizeInBytesMega << std::endl;
	//std::cout << "Size in Gigabytes:" << sizeInBytesGiga << std::endl;

	BufferRenderer aRenderer;
	
	WindowsProperties winProp;
	winProp.width = 1280;
	winProp.height = 720;

	aRenderer.InitalizeWindowAndDevice(winProp);
	aRenderer.InitalizeRenderer();
	
	aRenderer.Run();
	


//	std::cout << "Hello World!" << std::endl;

	system("pause");
	return 0;
}
