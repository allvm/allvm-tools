#ifndef ALLVMCONTEXT_ANCHOR_H
#define ALLVMCONTEXT_ANCHOR_H

#include "ALLVMContext.h"

namespace allvm {

char ALLVMContext::Anchor = 0;

ALLVMContext ALLVMContext::getAnchored(const char *Argv0) {
  return ALLVMContext::get(
      Argv0, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&Anchor)));
}

} // end namespace allvm

#else // ALLVMCONTEXT_ANCHOR_H

#error "Don't include this twice!"

#endif // ALLVMCONTEXT_ANCHOR_H
