#include"NDS.h"

NDS::NDS()
{
	//should init scheduler here (when fully implemented, i.e. i'm not lazy)
	m_initialise();
}

NDS::~NDS()
{

}

void NDS::run()
{
	//for now, just load in nds file and parse header - and print out ARM9/ARM7 load points, entry points, code size etc.
	//then need to map that data, could just use the bus's read/write functions lol.
	//also have to set R15 for both the ARM9/ARM7 so they start executing at the right place.
}

void NDS::notifyDetach()
{
	//todo
}

void NDS::m_initialise()
{
	//todo
}

void NDS::m_destroy()
{
	//todo
}

std::vector<uint8_t> NDS::readFile(const char* name)
{
	// open the file:
	std::ifstream file(name, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	std::vector<uint8_t> vec;
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<uint8_t>(file),
		std::istream_iterator<uint8_t>());

	file.close();

	return vec;
}