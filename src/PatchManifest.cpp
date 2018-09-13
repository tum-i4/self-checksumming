#include "PatchManifest.h"
#include "json.hpp"
#include <iostream>

void PatchManifest::readPatchManifest(std::string manifestFilePath) {
  using json = nlohmann::json;
  json root; // will contain the root value after parsing.
  std::ifstream stream(manifestFilePath, std::ifstream::binary);
  stream >> root;
  for (const auto& patch : root) {
    this->size_patches[patch["size_placeholder"].get<int>()] =
        patch["size_target"].get<int>();
    this->address_patches[patch["add_placeholder"].get<int>()] =
        patch["add_target"].get<int>();
    this->hash_patches[patch["hash_placeholder"].get<int>()] =
        patch["hash_target"].get<int>();
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
