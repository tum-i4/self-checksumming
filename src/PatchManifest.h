#include <fstream>
#include <map>

class PatchManifest {
public:
  std::map<int, int> address_patches, size_patches, hash_patches;
  void readPatchManifest(std::string manifestFilePath);
};
