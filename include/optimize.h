#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "settings.h"
#include "classes.h"

// Runs a TeraChem gas-phase optimization of the job-directory copy of the
// input and, on success, copies the optimized coordinates into `mol`
// (returns true). On failure `mol` is unchanged and false is returned.
bool TeraChemOpt(Settings settings, Molecule &mol);

#endif