#include "PatchManifest.h"
void PatchManifest::readPatchManifest(std::string manifestFilePath) {
  Json::Value root; // will contain the root value after parsing.
  std::ifstream stream(manifestFilePath, std::ifstream::binary);
  stream >> root;
  for (const Json::Value &patch : root) {
    this->size_patches[patch["size_placeholder"].asInt()] =
        patch["size_target"].asInt();
    this->address_patches[patch["add_placeholder"].asInt()] =
        patch["add_target"].asInt();
    this->hash_patches[patch["hash_placeholder"].asInt()] =
        patch["hash_target"].asInt();
  }
}
int main() {

  PatchManifest patchManifest;
  patchManifest.readPatchManifest("patch_guide");

  std::cout << patchManifest.hash_patches.size() << "\n";
  std::cout << patchManifest.size_patches.size() << "\n";
  std::cout << patchManifest.address_patches.size() << "\n";
  // std::string encoding = root.get("encoding", "UTF-8" ).asString();
  // std::cout << encoding << "\n";
  return 0;
}
