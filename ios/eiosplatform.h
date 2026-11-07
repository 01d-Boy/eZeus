#ifndef EIOSPLATFORM_H
#define EIOSPLATFORM_H

#include <string>

// Returns the app's Documents directory (visible in the Files app),
// with a trailing slash. Implemented in eiosplatform.mm.
std::string eIosDocumentsDir();

#endif
