#ifndef ALLVM_RESOURCEANCHOR_H
#define ALLVM_RESOURCEANCHOR_H

#include "allvm/ResourcePaths.h"

namespace allvm {

char ResourcePaths::Anchor = 0;

ResourcePaths ResourcePaths::getAnchored(const char *Argv0) {
  return ResourcePaths::get(
      Argv0, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&Anchor)));
}

} // end namespace allvm

#else // ALLVM_RESOURCEANCHOR_H

#error "Don't include this twice!"

#endif // ALLVM_RESOURCEANCHOR_H
