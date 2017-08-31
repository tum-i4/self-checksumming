#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <fstream>
#include <iostream>
class PatchManifest {
	public:
	std::map<int, int> address_patches, size_patches, hash_patches;
	void readPatchManifest(std::string manifestFilePath);
};
