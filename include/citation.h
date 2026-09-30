#ifndef AUTOPARAMS_CITATION_H
#define AUTOPARAMS_CITATION_H

#include "settings.h"

// 2026-09-30: every run that gets past the command-line checks writes
// <job_dir>/citation.txt -- how to cite AutoParams, TeraChem (which computes
// the electrostatic potential for the RESP fit, and runs -o/--optimize) and
// the RESP method -- and, when the process ends, prints
// "Citation details: see autoparams.NNNN/citation.txt" as the last line on
// stdout and in autoparams.NNNN.out.
void write_citation_file(const Settings &settings);

#endif
